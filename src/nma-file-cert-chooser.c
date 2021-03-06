// SPDX-License-Identifier: LGPL-2.1+
/* NetworkManager Applet -- allow user control over networking
 *
 * Dan Williams <dcbw@redhat.com>
 * Lubomir Rintel <lkundrak@v3.sk>
 *
 * Copyright (C) 2015,2017 Red Hat, Inc.
 */

#include "nm-default.h"
#include "nma-private.h"
#include "nma-cert-chooser-private.h"
#include "utils.h"
#include "nma-ui-utils.h"

#define NMA_FILE_CERT_CHOOSER_GET_PRIVATE(self) (&(_NM_GET_PRIVATE (self, NMACertChooser, NMA_IS_CERT_CHOOSER)->_sub.file))

#if GTK_CHECK_VERSION(3,90,0)
#define gtk3_widget_set_no_show_all(widget, show)
#else
#define gtk3_widget_set_no_show_all(widget, show) gtk_widget_set_no_show_all (widget, show);
#endif

static void
set_key_password (NMACertChooser *cert_chooser, const gchar *password)
{
	NMAFileCertChooserPrivate *priv = NMA_FILE_CERT_CHOOSER_GET_PRIVATE (cert_chooser);

	g_return_if_fail (priv->key_password != NULL);
	gtk_editable_set_text (GTK_EDITABLE (priv->key_password), password);
}

static const gchar *
get_key_password (NMACertChooser *cert_chooser)
{
	NMAFileCertChooserPrivate *priv = NMA_FILE_CERT_CHOOSER_GET_PRIVATE (cert_chooser);

	g_return_val_if_fail (priv->key_password != NULL, NULL);
	return gtk_editable_get_text (GTK_EDITABLE (priv->key_password));
}

static void
set_key_uri (NMACertChooser *cert_chooser, const gchar *uri)
{
	NMAFileCertChooserPrivate *priv = NMA_FILE_CERT_CHOOSER_GET_PRIVATE (cert_chooser);

	if (uri)
		gtk_file_chooser_set_uri (GTK_FILE_CHOOSER (priv->key_button), uri);
}

static gchar *
get_key_uri (NMACertChooser *cert_chooser)
{
	NMAFileCertChooserPrivate *priv = NMA_FILE_CERT_CHOOSER_GET_PRIVATE (cert_chooser);

	return gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (priv->key_button));
}

static void
clear_key (NMACertChooser *cert_chooser)
{
	NMAFileCertChooserPrivate *priv = NMA_FILE_CERT_CHOOSER_GET_PRIVATE (cert_chooser);

	gtk_file_chooser_unselect_all (GTK_FILE_CHOOSER (priv->key_button));
}

static void
set_cert_uri (NMACertChooser *cert_chooser, const gchar *uri)
{
	NMAFileCertChooserPrivate *priv = NMA_FILE_CERT_CHOOSER_GET_PRIVATE (cert_chooser);

	if (uri)
		gtk_file_chooser_set_uri (GTK_FILE_CHOOSER (priv->cert_button), uri);
}

static void
clear_cert (NMACertChooser *cert_chooser)
{
	NMAFileCertChooserPrivate *priv = NMA_FILE_CERT_CHOOSER_GET_PRIVATE (cert_chooser);

	gtk_file_chooser_unselect_all (GTK_FILE_CHOOSER (priv->cert_button));
}

static gchar *
get_cert_uri (NMACertChooser *cert_chooser)
{
	NMAFileCertChooserPrivate *priv = NMA_FILE_CERT_CHOOSER_GET_PRIVATE (cert_chooser);

	return gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (priv->cert_button));
}

static void
add_to_size_group (NMACertChooser *cert_chooser, GtkSizeGroup *group)
{
	NMAFileCertChooserPrivate *priv = NMA_FILE_CERT_CHOOSER_GET_PRIVATE (cert_chooser);

	gtk_size_group_add_widget (group, priv->cert_button_label);
	gtk_size_group_add_widget (group, priv->key_button_label);
	gtk_size_group_add_widget (group, priv->key_password_label);
}

static gboolean
validate (NMACertChooser *cert_chooser, GError **error)
{
	NMAFileCertChooserPrivate *priv = NMA_FILE_CERT_CHOOSER_GET_PRIVATE (cert_chooser);
	GError *local = NULL;
	char *tmp;

	tmp = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (priv->cert_button));
	if (tmp) {
		g_free (tmp);
	} else {
		g_set_error_literal (error, NMA_ERROR, NMA_ERROR_GENERIC, _("No certificate set"));
		return FALSE;
	}

	g_signal_emit_by_name (cert_chooser, "cert-validate", &local);
	if (local) {
		widget_set_error (priv->cert_button);
		g_propagate_error (error, local);
		return FALSE;
	} else {
		widget_unset_error (priv->cert_button);
	}

	if (gtk_widget_get_visible (priv->key_button)) {
		tmp = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (priv->key_button));
		if (tmp) {
			g_free (tmp);
		} else {
			g_set_error_literal (error, NMA_ERROR, NMA_ERROR_GENERIC, _("No key set"));
			return FALSE;
		}

		g_signal_emit_by_name (cert_chooser, "key-validate", &local);
		if (local) {
			widget_set_error (priv->key_button);
			g_propagate_error (error, local);
			return FALSE;
		} else {
			widget_unset_error (priv->key_button);
		}

		g_signal_emit_by_name (cert_chooser, "key-password-validate", &local);
		if (local) {
			widget_set_error (priv->key_password);
			g_propagate_error (error, local);
			return FALSE;
		} else {
			widget_unset_error (priv->key_password);
		}
	}

	return TRUE;
}

static void
setup_key_password_storage (NMACertChooser *cert_chooser,
                            NMSettingSecretFlags initial_flags,
                            NMSetting *setting,
                            const char *password_flags_name,
                            gboolean with_not_required,
                            gboolean ask_mode)
{
	NMAFileCertChooserPrivate *priv = NMA_FILE_CERT_CHOOSER_GET_PRIVATE (cert_chooser);

	nma_utils_setup_password_storage (priv->key_password,
	                                  initial_flags,
	                                  setting,
	                                  password_flags_name,
	                                  with_not_required,
	                                  ask_mode);
}

static void
update_key_password_storage (NMACertChooser *cert_chooser,
                             NMSettingSecretFlags secret_flags,
                             NMSetting *setting,
                             const char *password_flags_name)
{
	NMAFileCertChooserPrivate *priv = NMA_FILE_CERT_CHOOSER_GET_PRIVATE (cert_chooser);

	nma_utils_update_password_storage (priv->key_password,
	                                   secret_flags,
	                                   setting,
	                                   password_flags_name);
}


static NMSettingSecretFlags
get_key_password_flags (NMACertChooser *cert_chooser)
{
	NMAFileCertChooserPrivate *priv = NMA_FILE_CERT_CHOOSER_GET_PRIVATE (cert_chooser);

	return nma_utils_menu_to_secret_flags (priv->key_password);
}

static void
reset_filter (GtkWidget *widget, GParamSpec *spec, gpointer user_data)
{
	if (!gtk_file_chooser_get_filter (GTK_FILE_CHOOSER (widget))) {
		g_signal_handlers_block_by_func (widget, reset_filter, user_data);
		gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (widget), GTK_FILE_FILTER (user_data));
		g_signal_handlers_unblock_by_func (widget, reset_filter, user_data);
	}
}

static void
cert_changed_cb (GtkFileChooserButton *file_chooser_button, gpointer user_data)
{
	NMAFileCertChooserPrivate *priv = NMA_FILE_CERT_CHOOSER_GET_PRIVATE (NMA_CERT_CHOOSER (user_data));

	if (gtk_widget_get_visible (priv->key_button)) {
		gboolean sensitive = FALSE;
		gs_free char *cert = NULL;

		cert = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (priv->cert_button));
		if (cert && *cert) {
			if (nm_utils_file_is_pkcs12 (cert)) {
				gs_free char *key = NULL;

				key = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (priv->key_button));
				if (!nm_streq0 (cert, key))
					gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (priv->key_button), cert);
			} else
				sensitive = TRUE;
		} else
			gtk_file_chooser_unselect_all (GTK_FILE_CHOOSER (priv->key_button));

		gtk_widget_set_sensitive (priv->key_button, sensitive);
		gtk_widget_set_sensitive (priv->key_button_label, sensitive);
	}

	g_signal_emit_by_name (user_data, "changed");
}

static void
key_changed_cb (GtkFileChooserButton *file_chooser_button, gpointer user_data)
{
	NMAFileCertChooserPrivate *priv = NMA_FILE_CERT_CHOOSER_GET_PRIVATE (NMA_CERT_CHOOSER (user_data));
	gboolean sensitive = FALSE;
	gs_free char *key = NULL;

	key = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (priv->key_button));
	if (key && *key) {
		if (nm_utils_file_is_pkcs12 (key)) {
			gs_free char *cert = NULL;

			cert = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (priv->cert_button));
			if (!nm_streq0 (cert, key))
				gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (priv->cert_button), key);
			gtk_widget_set_sensitive (priv->key_button, FALSE);
			gtk_widget_set_sensitive (priv->key_button_label, FALSE);
		}
		sensitive = TRUE;
	}
	gtk_editable_set_text (GTK_EDITABLE (priv->key_password), "");
	gtk_widget_set_sensitive (priv->key_password, sensitive);
	gtk_widget_set_sensitive (priv->key_password_label, sensitive);
	widget_unset_error (priv->key_password);
	g_signal_emit_by_name (user_data, "changed");
}

static void
key_password_changed_cb (GtkEntry *entry, gpointer user_data)
{
	g_signal_emit_by_name (user_data, "changed");
}

static void
show_toggled_cb (GtkCheckButton *button, gpointer user_data)
{
	NMAFileCertChooserPrivate *priv = NMA_FILE_CERT_CHOOSER_GET_PRIVATE (NMA_CERT_CHOOSER (user_data));

	gtk_entry_set_visibility (GTK_ENTRY (priv->key_password),
	                          gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)));
}

static void
set_title (NMACertChooser *cert_chooser, const gchar *title)
{
	NMAFileCertChooserPrivate *priv = NMA_FILE_CERT_CHOOSER_GET_PRIVATE (cert_chooser);
	gchar *text;

	text = g_strdup_printf (_("Choose a key for %s Certificate"), title);
	gtk_file_chooser_button_set_title (GTK_FILE_CHOOSER_BUTTON (priv->key_button), text);
	g_free (text);

	text = g_strdup_printf (_("%s private _key"), title);
	gtk_label_set_text_with_mnemonic (GTK_LABEL (priv->key_button_label), text);
	g_free (text);

	text = g_strdup_printf (_("%s key _password"), title);
	gtk_label_set_text_with_mnemonic (GTK_LABEL (priv->key_password_label), text);
	g_free (text);

	text = g_strdup_printf (_("Choose %s Certificate"), title);
	gtk_file_chooser_button_set_title (GTK_FILE_CHOOSER_BUTTON (priv->cert_button), text);
	g_free (text);

	text = g_strdup_printf (_("%s _certificate"), title);
	gtk_label_set_text_with_mnemonic (GTK_LABEL (priv->cert_button_label), text);
	g_free (text);
}

static void
set_flags (NMACertChooser *cert_chooser, NMACertChooserFlags flags)
{
	NMAFileCertChooserPrivate *priv = NMA_FILE_CERT_CHOOSER_GET_PRIVATE (cert_chooser);

	if (flags & NMA_CERT_CHOOSER_FLAG_CERT) {
		gtk_widget_hide (priv->key_button);
		gtk_widget_hide (priv->key_button_label);
		gtk_widget_hide (priv->key_password);
		gtk_widget_hide (priv->key_password_label);
		gtk_widget_hide (priv->show_password);
	}

	if (flags & NMA_CERT_CHOOSER_FLAG_PASSWORDS) {
		gtk_widget_hide (priv->cert_button);
		gtk_widget_hide (priv->cert_button_label);
		gtk_widget_hide (priv->key_button);
		gtk_widget_hide (priv->key_button_label);
	}
}

static void
init (NMACertChooser *cert_chooser)
{
	NMAFileCertChooserPrivate *priv = NMA_FILE_CERT_CHOOSER_GET_PRIVATE (cert_chooser);
	GtkFileFilter *filter;

	gtk_grid_insert_column (GTK_GRID (cert_chooser), 2);
	gtk_grid_set_row_spacing (GTK_GRID (cert_chooser), 6);
	gtk_grid_set_column_spacing (GTK_GRID (cert_chooser), 6);

	/* The key chooser */
	gtk_grid_insert_row (GTK_GRID (cert_chooser), 0);

	priv->key_button = g_object_new (GTK_TYPE_FILE_CHOOSER_BUTTON,
	                                  "action", GTK_FILE_CHOOSER_ACTION_OPEN,
	                                  "filter", utils_key_filter (),
	                                  "local-only", TRUE,
	                                  NULL);
	gtk_grid_attach (GTK_GRID (cert_chooser), priv->key_button, 1, 0, 1, 1);
	gtk_widget_set_hexpand (priv->key_button, TRUE);
	gtk_widget_set_sensitive (priv->key_button, FALSE);
	gtk_widget_show (priv->key_button);
	gtk3_widget_set_no_show_all (priv->key_button, TRUE);

	g_signal_connect (priv->key_button, "selection-changed",
	                  G_CALLBACK (key_changed_cb), cert_chooser);

	priv->key_button_label = gtk_label_new (NULL);
	g_object_set (priv->key_button_label, "xalign", (gfloat) 1, NULL);
	gtk_label_set_mnemonic_widget (GTK_LABEL (priv->key_button_label), priv->key_button);
	gtk_grid_attach (GTK_GRID (cert_chooser), priv->key_button_label, 0, 0, 1, 1);
	gtk_widget_set_sensitive (priv->key_button_label, FALSE);
	gtk_widget_show (priv->key_button_label);
	gtk3_widget_set_no_show_all (priv->key_button_label, TRUE);

	/* The key password entry */
	gtk_grid_insert_row (GTK_GRID (cert_chooser), 1);

	priv->key_password = gtk_entry_new ();
	gtk_entry_set_visibility (GTK_ENTRY (priv->key_password), FALSE);
	gtk_grid_attach (GTK_GRID (cert_chooser), priv->key_password, 1, 1, 1, 1);
	gtk_widget_set_hexpand (priv->key_password, TRUE);
	gtk_widget_set_sensitive (priv->key_password, FALSE);
	gtk_widget_show (priv->key_password);
	gtk3_widget_set_no_show_all (priv->key_password, TRUE);

	g_signal_connect (priv->key_password, "changed",
	                  G_CALLBACK (key_password_changed_cb), cert_chooser);

	priv->key_password_label = gtk_label_new (NULL);
	g_object_set (priv->key_password_label, "xalign", (gfloat) 1, NULL);
	gtk_label_set_mnemonic_widget (GTK_LABEL (priv->key_password_label), priv->key_password);
	gtk_grid_attach (GTK_GRID (cert_chooser), priv->key_password_label, 0, 1, 1, 1);
	gtk_widget_set_sensitive (priv->key_password_label, FALSE);
	gtk_widget_show (priv->key_password_label);
	gtk3_widget_set_no_show_all (priv->key_password_label, TRUE);

	/* Show password */
	gtk_grid_insert_row (GTK_GRID (cert_chooser), 2);
	priv->show_password = gtk_check_button_new_with_mnemonic _("Sho_w password");
	gtk_grid_attach (GTK_GRID (cert_chooser), priv->show_password, 1, 2, 1, 1);
	gtk_widget_show (priv->show_password);
	gtk3_widget_set_no_show_all (priv->show_password, TRUE);
	g_signal_connect (priv->show_password, "toggled",
	                  G_CALLBACK (show_toggled_cb), cert_chooser);

	/* The certificate chooser */
	gtk_grid_insert_row (GTK_GRID (cert_chooser), 0);

	filter = utils_cert_filter ();
	priv->cert_button = g_object_new (GTK_TYPE_FILE_CHOOSER_BUTTON,
	                                  "action", GTK_FILE_CHOOSER_ACTION_OPEN,
	                                  "filter", filter,
	                                  "local-only", TRUE,
	                                  NULL);
	gtk_grid_attach (GTK_GRID (cert_chooser), priv->cert_button, 1, 0, 1, 1);
	gtk_widget_set_hexpand (priv->cert_button, TRUE);
	gtk_widget_show (priv->cert_button);
	gtk3_widget_set_no_show_all (priv->cert_button, TRUE);

	/* For some reason, GTK+ calls set_current_filter (..., NULL) from
	 * gtkfilechooserdefault.c::show_and_select_files_finished_loading() on our
	 * dialog; so force-reset the filter to what we want it to be whenever
	 * it gets cleared.
	 */
	g_signal_connect (priv->cert_button, "notify::filter",
	                  G_CALLBACK (reset_filter), filter);

	g_signal_connect (priv->cert_button, "selection-changed",
	                  G_CALLBACK (cert_changed_cb), cert_chooser);

	priv->cert_button_label = gtk_label_new (NULL);
	g_object_set (priv->cert_button_label, "xalign", (gfloat) 1, NULL);
	gtk_label_set_mnemonic_widget (GTK_LABEL (priv->cert_button_label), priv->cert_button);
	gtk_grid_attach (GTK_GRID (cert_chooser), priv->cert_button_label, 0, 0, 1, 1);
	gtk_widget_show (priv->cert_button_label);
	gtk3_widget_set_no_show_all (priv->cert_button_label, TRUE);
}

const NMACertChooserVtable nma_cert_chooser_vtable_file = {
	.init = init,

	.set_title = set_title,
	.set_flags = set_flags,

	.set_cert_uri = set_cert_uri,
	.get_cert_uri = get_cert_uri,
	.clear_cert   = clear_cert,
	.set_key_uri = set_key_uri,
	.get_key_uri = get_key_uri,
	.clear_key   = clear_key,
	.set_key_password = set_key_password,
	.get_key_password = get_key_password,

	.add_to_size_group = add_to_size_group,
	.validate = validate,

	.setup_key_password_storage = setup_key_password_storage,
	.update_key_password_storage = update_key_password_storage,
	.get_key_password_flags = get_key_password_flags,
};
