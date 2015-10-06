/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2015 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU Lesser General Public License Version 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "config.h"

#include <libgcab.h>
#include <glib/gstdio.h>
#include <gio/gunixinputstream.h>

#include "as-store-cab.h"

#ifndef GCabCabinet_autoptr
G_DEFINE_AUTOPTR_CLEANUP_FUNC(GCabCabinet, g_object_unref)
#endif

/**
 * as_store_cab_cb:
 **/
static gboolean
as_store_cab_cb (GCabFile *file, gpointer user_data)
{
	GPtrArray *filelist = (GPtrArray *) user_data;
	g_ptr_array_add (filelist, g_strdup (gcab_file_get_name (file)));
	return TRUE;
}

/**
 * as_store_cab_set_release_blobs:
 **/
static gboolean
as_store_cab_set_release_blobs (AsRelease *release, const gchar *tmp_path, GError **error)
{
	g_autofree gchar *asc_basename = NULL;
	g_autofree gchar *asc_fn = NULL;
	g_autofree gchar *rel_basename = NULL;
	g_autofree gchar *rel_fn = NULL;
	g_autoptr(GError) error_local = NULL;

	/* get the firmware filename */
	rel_basename = g_path_get_basename (as_release_get_filename (release));
	rel_fn = g_build_filename (tmp_path, rel_basename, NULL);

	/* add this information to the release objects */
	if (g_file_test (rel_fn, G_FILE_TEST_EXISTS)) {
		gchar *data = NULL;
		gsize data_len = 0;
		g_autoptr(GBytes) blob = NULL;

		if (!g_file_get_contents (rel_fn, &data, &data_len, &error_local)) {
			g_set_error (error,
				     AS_STORE_ERROR,
				     AS_STORE_ERROR_FAILED,
				     "failed to open %s: %s",
				     rel_fn, error_local->message);
			return FALSE;
		}

		/* this is the size of the firmware */
		if (as_release_get_size (release, AS_SIZE_KIND_INSTALLED) == 0) {
			as_release_set_size (release,
					     AS_SIZE_KIND_INSTALLED,
					     data_len);
		}

		/* set the data on the release object */
		blob = g_bytes_new_take (data, data_len);
		as_release_set_blob (release, rel_basename, blob);
	}

	/* if the signing file exists, set that too */
	asc_basename = g_strdup_printf ("%s.asc", rel_basename);
	asc_fn = g_build_filename (tmp_path, asc_basename, NULL);
	if (g_file_test (asc_fn, G_FILE_TEST_EXISTS)) {
		gchar *data = NULL;
		gsize data_len = 0;
		g_autoptr(GBytes) blob = NULL;

		if (!g_file_get_contents (asc_fn, &data, &data_len, &error_local)) {
			g_set_error (error,
				     AS_STORE_ERROR,
				     AS_STORE_ERROR_FAILED,
				     "failed to open %s: %s",
				     asc_fn, error_local->message);
			return FALSE;
		}

		/* set the data on the release object */
		blob = g_bytes_new_take (data, data_len);
		as_release_set_blob (release, asc_basename, blob);
	}
	return TRUE;
}

/**
 * as_store_cab_from_stream:
 **/
static gboolean
as_store_cab_from_stream (AsStore *store,
			  GInputStream *input_stream,
			  guint64 size,
			  GCancellable *cancellable,
			  GError **error)
{
	g_autoptr(GCabCabinet) gcab = NULL;
	g_autoptr(GError) error_local = NULL;
	g_autofree gchar *tmp_path = NULL;
	g_autoptr(GFile) tmp_file = NULL;
	g_autoptr(GPtrArray) filelist = NULL;
	g_autoptr(GPtrArray) apps = NULL;
	guint i;

	/* open the file */
	gcab = gcab_cabinet_new ();
	if (!gcab_cabinet_load (gcab, input_stream, NULL, &error_local)) {
		g_set_error (error,
			     AS_STORE_ERROR,
			     AS_STORE_ERROR_FAILED,
			     "cannot load .cab file: %s",
			     error_local->message);
		return FALSE;
	}

	/* decompress to /tmp */
	tmp_path = g_dir_make_tmp ("appstream-glib-XXXXXX", &error_local);
	if (tmp_path == NULL) {
		g_set_error (error,
			     AS_STORE_ERROR,
			     AS_STORE_ERROR_FAILED,
			     "failed to create temp dir: %s",
			     error_local->message);
		return FALSE;
	}

	/* extract the entire cab file */
	filelist = g_ptr_array_new_with_free_func (g_free);
	tmp_file = g_file_new_for_path (tmp_path);
	if (!gcab_cabinet_extract_simple (gcab, tmp_file,
					  as_store_cab_cb,
					  filelist, NULL, &error_local)) {
		g_set_error (error,
			     AS_STORE_ERROR,
			     AS_STORE_ERROR_FAILED,
			     "failed to extract .cab file: %s",
			     error_local->message);
		return FALSE;
	}

	/* loop through each file looking for components */
	apps = g_ptr_array_new_with_free_func ((GDestroyNotify) g_object_unref);
	for (i = 0; i < filelist->len; i++) {
		const gchar *fn;
		g_autofree gchar *tmp_fn = NULL;
		g_autoptr(AsApp) app = NULL;
		AsRelease *rel;

		/* debug */
		fn = g_ptr_array_index (filelist, i);
		g_debug ("found file %i\t%s", i, fn);

		/* if inf or metainfo, add */
		switch (as_app_guess_source_kind (fn)) {
		case AS_APP_SOURCE_KIND_METAINFO:
			tmp_fn = g_build_filename (tmp_path, fn, NULL);
			app = as_app_new ();
			if (!as_app_parse_file (app, tmp_fn,
						AS_APP_PARSE_FLAG_NONE, &error_local)) {
				g_set_error (error,
					     AS_STORE_ERROR,
					     AS_STORE_ERROR_FAILED,
					     "%s could not be loaded: %s",
					     tmp_fn,
					     error_local->message);
				return FALSE;
			}

			/* check release was valid */
			rel = as_app_get_release_default (app);
			if (rel == NULL) {
				g_set_error_literal (error,
						     AS_STORE_ERROR,
						     AS_STORE_ERROR_FAILED,
						     "no releases in metainfo file");
				return FALSE;
			}

			/* fix up legacy files */
			if (as_release_get_filename (rel) == NULL)
				as_release_set_filename (rel, "firmware.bin");

			/* this is the size of the cab file itself */
			if (size > 0 && as_release_get_size (rel, AS_SIZE_KIND_DOWNLOAD) == 0)
				as_release_set_size (rel, AS_SIZE_KIND_DOWNLOAD, size);

			g_ptr_array_add (apps, g_object_ref (app));
			break;
		case AS_APP_SOURCE_KIND_INF:
			/* FIXME: can we handle this? */
			break;
		default:
			break;
		}
	}

	/* add firmware blobs referenced by the metainfo or inf files */
	for (i = 0; i < apps->len; i++) {
		AsApp *app_tmp = AS_APP (g_ptr_array_index (apps, i));
		GPtrArray *releases = as_app_get_releases (app_tmp);
		guint j;
		for (j = 0; j < releases->len; j++) {
			AsRelease *rel = g_ptr_array_index (releases, j);
			if (!as_store_cab_set_release_blobs (rel, tmp_path, error))
				return FALSE;
		}
	}

	/* add any remaining components to the store */
	for (i = 0; i < apps->len; i++) {
		AsApp *app_tmp = AS_APP (g_ptr_array_index (apps, i));
		as_store_add_app (store, app_tmp);
	}

	/* delete temp files */
	for (i = 0; i < filelist->len; i++) {
		const gchar *fn;
		g_autofree gchar *tmp_fn = NULL;
		fn = g_ptr_array_index (filelist, i);
		tmp_fn = g_build_filename (tmp_path, fn, NULL);
		g_unlink (tmp_fn);
	}
	g_rmdir (tmp_path);

	/* success */
	return TRUE;
}

/**
 * as_store_cab_from_fd:
 **/
gboolean
as_store_cab_from_fd (AsStore *store,
		      int fd,
		      GCancellable *cancellable,
		      GError **error)
{
	guint64 size = NULL;
	g_autoptr(GError) error_local = NULL;
	g_autoptr(GInputStream) input_stream = NULL;
	g_autoptr(GInputStream) stream = NULL;

	/* GCab needs a GSeekable input stream, so buffer to RAM then load */
	stream = g_unix_input_stream_new (fd, TRUE);
	input_stream = g_memory_input_stream_new ();
	while (1) {
		g_autoptr(GBytes) data = NULL;
		data = g_input_stream_read_bytes (stream, 8192,
						  cancellable,
						  &error_local);
		if (g_bytes_get_size (data) == 0)
			break;
		if (data == NULL) {
			g_set_error_literal (error,
					     AS_STORE_ERROR,
					     AS_STORE_ERROR_FAILED,
					     error_local->message);
			return FALSE;
		}
		size += g_bytes_get_size (data);
		g_memory_input_stream_add_bytes (G_MEMORY_INPUT_STREAM (input_stream), data);
	}

	/* parse */
	return as_store_cab_from_stream (store, input_stream, size, cancellable, error);
}

/**
 * as_store_cab_from_file:
 **/
gboolean
as_store_cab_from_file (AsStore *store,
			GFile *file,
			GCancellable *cancellable,
			GError **error)
{
	guint64 size;
	g_autoptr(GError) error_local = NULL;
	g_autoptr(GFileInfo) info = NULL;
	g_autoptr(GInputStream) input_stream = NULL;
	g_autofree gchar *filename = NULL;
	g_autofree gchar *origin = NULL;

	/* set origin */
	origin = g_file_get_basename (file);
	as_store_set_origin (store, origin);

	/* get size */
	info = g_file_query_info (file, G_FILE_ATTRIBUTE_STANDARD_SIZE,
				  G_FILE_QUERY_INFO_NONE, cancellable, &error_local);
	if (info == NULL) {
		filename = g_file_get_path (file);
		g_set_error (error,
			     AS_STORE_ERROR,
			     AS_STORE_ERROR_FAILED,
			     "Failed to get info for %s: %s",
			     filename, error_local->message);
		return FALSE;
	}
	size = g_file_info_get_attribute_uint64 (info, G_FILE_ATTRIBUTE_STANDARD_SIZE);

	/* open file */
	input_stream = G_INPUT_STREAM (g_file_read (file, cancellable, &error_local));
	if (input_stream == NULL) {
		filename = g_file_get_path (file);
		g_set_error (error,
			     AS_STORE_ERROR,
			     AS_STORE_ERROR_FAILED,
			     "Failed to open %s: %s",
			     filename, error_local->message);
		return FALSE;
	}

	/* parse */
	return as_store_cab_from_stream (store, input_stream, size, cancellable, error);
}
