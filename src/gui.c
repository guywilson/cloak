#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <gtk/gtk.h>

#include "cloak.h"
#include "cloak_types.h"
#include "utils.h"

#define APPLICATION_ID "com.guy.cloak.cloak"

typedef enum {
    actionMerge,
    actionExtract
}
cloak_action;

typedef struct {
    GtkBuilder *        builder;

    cloak_action        action;

    char *              pszSourceImageFile;
    char *              pszOutputFile;
    char *              pszKeystreamFile;

    uint8_t *           key;
    uint32_t            keyLength;

    merge_quality       quality;
    encryption_algo     algo;
}
CLOAK_INFO;

static CLOAK_INFO       _cloakInfo;


static void refreshCapacity()
{
    GtkWidget *     capacityLabel;
    uint32_t        imageCapacity;
    char            capacityText[64];

    imageCapacity = (getImageCapacity(_cloakInfo.pszSourceImageFile, _cloakInfo.quality) / 1024);

    sprintf(capacityText, "Capacity: %u Kb", imageCapacity);

    capacityLabel = gtk_builder_get_object(_cloakInfo.builder, "capacityLabel");
    gtk_label_set_label(capacityLabel, capacityText);
}

static void handleImageOpen(GtkNativeDialog * dialog, int response)
{
    GFile *         file;
    GdkPixbuf *     pixBuf;
    GtkWidget *     image;

    if (response == GTK_RESPONSE_ACCEPT) {
        GtkFileChooser * chooser = GTK_FILE_CHOOSER(dialog);

        file = gtk_file_chooser_get_file(chooser);
        _cloakInfo.pszSourceImageFile = g_file_get_path(file);
        g_print("Got file %s\n", _cloakInfo.pszSourceImageFile);

        image = gtk_builder_get_object(_cloakInfo.builder, "image");
        pixBuf = gdk_pixbuf_new_from_file(_cloakInfo.pszSourceImageFile, NULL);

        gtk_image_set_from_pixbuf(GTK_IMAGE(image), pixBuf);

        refreshCapacity();
    }

    g_object_unref(dialog);
}

static void handleMergeOpen(GtkNativeDialog * dialog, int response)
{
    GFile *         file;
    char *          pszSecretFilename;

    if (response == GTK_RESPONSE_ACCEPT) {
        GtkFileChooser * chooser = GTK_FILE_CHOOSER(dialog);

        file = gtk_file_chooser_get_file(chooser);
        pszSecretFilename = g_file_get_path(file);
        g_print("Got file %s\n", pszSecretFilename);
    }

    g_object_unref(dialog);
}

static void handleExtractSave(GtkNativeDialog * dialog, int response)
{
    GFile *         file;
    char *          pszSecretFilename;

    if (response == GTK_RESPONSE_ACCEPT) {
        GtkFileChooser * chooser = GTK_FILE_CHOOSER(dialog);

        file = gtk_file_chooser_get_file(chooser);
        pszSecretFilename = g_file_get_path(file);
        g_print("Got file %s\n", pszSecretFilename);
    }

    g_object_unref(dialog);
}

static void handleHighQualityToggle(GtkWidget * radio, gpointer data)
{
    if (gtk_check_button_get_active(GTK_CHECK_BUTTON(radio))) {
        _cloakInfo.quality = quality_high;
        refreshCapacity();
    }
}

static void handleMediumQualityToggle(GtkWidget * radio, gpointer data)
{
    if (gtk_check_button_get_active(GTK_CHECK_BUTTON(radio))) {
        _cloakInfo.quality = quality_medium;
        refreshCapacity();
    }
}

static void handleLowQualityToggle(GtkWidget * radio, gpointer data)
{
    if (gtk_check_button_get_active(GTK_CHECK_BUTTON(radio))) {
        _cloakInfo.quality = quality_low;
        refreshCapacity();
    }
}

static void handleAesEncryptionToggle(GtkWidget * radio, gpointer data)
{
    GtkWidget *     aesPasswordField;
    GtkWidget *     xorKeystreamFileField;
    GtkWidget *     xorBrowseButton;

    if (gtk_check_button_get_active(GTK_CHECK_BUTTON(radio))) {
        aesPasswordField = gtk_builder_get_object(_cloakInfo.builder, "aesPasswordField");
        xorKeystreamFileField = gtk_builder_get_object(_cloakInfo.builder, "xorKeystreamField");
        xorBrowseButton = gtk_builder_get_object(_cloakInfo.builder, "xorBrowseButton");

        gtk_widget_set_sensitive(aesPasswordField, TRUE);
        gtk_widget_set_sensitive(xorKeystreamFileField, FALSE);
        gtk_widget_set_sensitive(xorBrowseButton, FALSE);
    }
}

static void handleXorEncryptionToggle(GtkWidget * radio, gpointer data)
{
    GtkWidget *     aesPasswordField;
    GtkWidget *     xorKeystreamFileField;
    GtkWidget *     xorBrowseButton;

    if (gtk_check_button_get_active(GTK_CHECK_BUTTON(radio))) {
        aesPasswordField = gtk_builder_get_object(_cloakInfo.builder, "aesPasswordField");
        xorKeystreamFileField = gtk_builder_get_object(_cloakInfo.builder, "xorKeystreamField");
        xorBrowseButton = gtk_builder_get_object(_cloakInfo.builder, "xorBrowseButton");

        gtk_widget_set_sensitive(aesPasswordField, FALSE);
        gtk_widget_set_sensitive(xorKeystreamFileField, TRUE);
        gtk_widget_set_sensitive(xorBrowseButton, TRUE);
    }
}

static void handleNoneEncryptionToggle(GtkWidget * radio, gpointer data)
{
    GtkWidget *     aesPasswordField;
    GtkWidget *     xorKeystreamFileField;
    GtkWidget *     xorBrowseButton;

    if (gtk_check_button_get_active(GTK_CHECK_BUTTON(radio))) {
        aesPasswordField = gtk_builder_get_object(_cloakInfo.builder, "aesPasswordField");
        xorKeystreamFileField = gtk_builder_get_object(_cloakInfo.builder, "xorKeystreamField");
        xorBrowseButton = gtk_builder_get_object(_cloakInfo.builder, "xorBrowseButton");

        gtk_widget_set_sensitive(aesPasswordField, FALSE);
        gtk_widget_set_sensitive(xorKeystreamFileField, FALSE);
        gtk_widget_set_sensitive(xorBrowseButton, FALSE);
    }
}

static void handleGoButtonClick(GtkWidget * widget, gpointer data)
{
    GtkFileChooserNative *          openDialog;
    GtkWidget *                     mergeActionRadio;
    GtkWidget *                     extractActionRadio;

    g_print("Hello World - Go clicked\n");

    mergeActionRadio = gtk_builder_get_object(_cloakInfo.builder, "mergeActionRadio");
    extractActionRadio = gtk_builder_get_object(_cloakInfo.builder, "extractActionRadio");

    if (gtk_check_button_get_active(GTK_CHECK_BUTTON(mergeActionRadio))) {
        openDialog = gtk_file_chooser_native_new(
                            "Open a secret file", 
                            (GtkWindow *)data, 
                            GTK_FILE_CHOOSER_ACTION_OPEN, 
                            "_Open",
                            "_Cancel");

        g_signal_connect(openDialog, "response", G_CALLBACK(handleMergeOpen), NULL);
        gtk_native_dialog_show(GTK_NATIVE_DIALOG(openDialog));
    }
    else if (gtk_check_button_get_active(GTK_CHECK_BUTTON(extractActionRadio))) {
        openDialog = gtk_file_chooser_native_new(
                            "Save the secret file", 
                            (GtkWindow *)data, 
                            GTK_FILE_CHOOSER_ACTION_SAVE, 
                            "Save _As",
                            "_Cancel");

        g_signal_connect(openDialog, "response", G_CALLBACK(handleExtractSave), NULL);
        gtk_native_dialog_show(GTK_NATIVE_DIALOG(openDialog));
    }
}

static void handleBrowseButtonClick(GtkWidget * widget, gpointer data)
{
    g_print("Hello World - Browse clicked\n");
}

static void handleOpenButtonClick(GtkWidget * widget, gpointer data)
{
    GtkFileChooserNative *          openDialog;
    GtkFileFilter *                 imageFilter;

    g_print("Hello World - Open clicked\n");

    imageFilter = gtk_file_filter_new();

    gtk_file_filter_add_suffix(imageFilter, "png");
    gtk_file_filter_add_suffix(imageFilter, "bmp");

    openDialog = gtk_file_chooser_native_new(
                        "Open an image file", 
                        (GtkWindow *)data, 
                        GTK_FILE_CHOOSER_ACTION_OPEN, 
                        "_Open",
                        "_Cancel");

    gtk_file_chooser_add_filter(openDialog, imageFilter);

    g_signal_connect(openDialog, "response", G_CALLBACK(handleImageOpen), NULL);
    gtk_native_dialog_show(GTK_NATIVE_DIALOG(openDialog));
}

static void activate(GtkApplication * app, gpointer user_data)
{
    GtkBuilder *        builder;
    GtkWidget *         mainWindow;
    GtkWidget *         mainGrid;
    GtkWidget *         openButton;
    GtkWidget *         image;
    GtkWidget *         capacityLabel;
    GtkWidget *         imageBox;
    GtkWidget *         actionBox;
    GtkWidget *         imageFrame;
    GtkWidget *         actionFrame;
    GtkWidget *         goButton;
    GtkWidget *         encryptionFrame;
    GtkWidget *         encryptionGrid;
    GtkWidget *         aesEncryptionRadio;
    GtkWidget *         xorEncryptionRadio;
    GtkWidget *         noneEncryptionRadio;
    GtkWidget *         aesPasswordField;
    GtkWidget *         xorKeystreamField;
    GtkWidget *         xorBrowseButton;
    GtkWidget *         qualityFrame;
    GtkWidget *         qualityBox;
    GtkWidget *         highQualityRadio;
    GtkWidget *         mediumQualityRadio;
    GtkWidget *         lowQualityRadio;
    GdkPixbuf *         pixbuf;

    builder = gtk_builder_new();
    gtk_builder_add_from_file(builder, "builder.ui", NULL);

    _cloakInfo.builder = builder;

    mainWindow = gtk_builder_get_object(builder, "mainWindow");
    gtk_window_set_application(GTK_WINDOW(mainWindow), app);

    openButton = gtk_builder_get_object(builder, "openButton");
    g_signal_connect(openButton, "clicked", G_CALLBACK(handleOpenButtonClick), NULL);

    goButton = gtk_builder_get_object(builder, "goButton");
    g_signal_connect(goButton, "clicked", G_CALLBACK(handleGoButtonClick), NULL);

    xorBrowseButton = gtk_builder_get_object(builder, "xorBrowseButton");
    g_signal_connect(xorBrowseButton, "clicked", G_CALLBACK(handleBrowseButtonClick), NULL);

    highQualityRadio = gtk_builder_get_object(builder, "highQualityRadio");
    g_signal_connect(highQualityRadio, "toggled", G_CALLBACK(handleHighQualityToggle), NULL);
    _cloakInfo.quality = quality_high;

    mediumQualityRadio = gtk_builder_get_object(builder, "mediumQualityRadio");
    g_signal_connect(mediumQualityRadio, "toggled", G_CALLBACK(handleMediumQualityToggle), NULL);

    lowQualityRadio = gtk_builder_get_object(builder, "lowQualityRadio");
    g_signal_connect(lowQualityRadio, "toggled", G_CALLBACK(handleLowQualityToggle), NULL);

    aesEncryptionRadio = gtk_builder_get_object(builder, "aesEncryptionRadio");
    g_signal_connect(aesEncryptionRadio, "toggled", G_CALLBACK(handleAesEncryptionToggle), NULL);

    xorEncryptionRadio = gtk_builder_get_object(builder, "xorEncryptionRadio");
    g_signal_connect(xorEncryptionRadio, "toggled", G_CALLBACK(handleXorEncryptionToggle), NULL);

    noneEncryptionRadio = gtk_builder_get_object(builder, "noneEncryptionRadio");
    g_signal_connect(noneEncryptionRadio, "toggled", G_CALLBACK(handleNoneEncryptionToggle), NULL);

    pixbuf = gdk_pixbuf_new_from_file_at_size("./initialImage.png", 400, 400, NULL);
    image = gtk_builder_get_object(builder, "image");
    gtk_image_set_from_pixbuf(GTK_IMAGE(image), pixbuf);
    gtk_widget_set_size_request(image, 400, 400);

    gtk_widget_show(mainWindow);

    // mainWindow = gtk_application_window_new(app);
    // gtk_window_set_title(GTK_WINDOW(mainWindow), "Cloak");

    // mainGrid = gtk_grid_new();

    // imageFrame = gtk_frame_new(NULL);
    // imageBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

    // pixbuf = gdk_pixbuf_new_from_file_at_size("./initialImage.png", 400, 400, NULL);
    // image = gtk_image_new_from_pixbuf(pixbuf);

    // gtk_widget_set_size_request(image, 400, 400);

    // capacityLabel = gtk_label_new_with_mnemonic("_Capacity: ");
    // gtk_widget_set_halign(capacityLabel, GTK_ALIGN_START);

    // gtk_box_append(GTK_BOX(imageBox), image);
    // gtk_box_append(GTK_BOX(imageBox), capacityLabel);

    // gtk_box_set_homogeneous(GTK_BOX(imageBox), FALSE);

    // gtk_widget_set_margin_top(imageBox, 10);
    // gtk_widget_set_margin_bottom(imageBox, 10);
    // gtk_widget_set_margin_start(imageBox, 10);
    // gtk_widget_set_margin_end(imageBox, 10);

    // gtk_frame_set_child(GTK_FRAME(imageFrame), imageBox);

    // gtk_widget_set_margin_top(imageFrame, 10);
    // gtk_widget_set_margin_bottom(imageFrame, 10);
    // gtk_widget_set_margin_start(imageFrame, 10);
    // gtk_widget_set_margin_end(imageFrame, 10);

    // actionFrame = gtk_frame_new(NULL);
    // actionBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

    // mergeButton = gtk_button_new_with_mnemonic("_Merge...");
    // extractButton = gtk_button_new_with_mnemonic("_Extract...");

    // g_signal_connect(mergeButton, "clicked", G_CALLBACK(handleMergeButtonClick), mainWindow);
    // g_signal_connect(extractButton, "clicked", G_CALLBACK(handleExtractButtonClick), mainWindow);

    // gtk_button_set_use_underline(GTK_BUTTON(mergeButton), TRUE);
    // gtk_button_set_use_underline(GTK_BUTTON(extractButton), TRUE);

    // gtk_widget_set_tooltip_text(mergeButton, "Merge a secret file with the image");
    // gtk_widget_set_tooltip_text(extractButton, "Extract a secret file from the image");

    // gtk_box_append(GTK_BOX(actionBox), mergeButton);
    // gtk_box_append(GTK_BOX(actionBox), extractButton);

    // gtk_box_set_homogeneous(GTK_BOX(actionBox), FALSE);

    // gtk_widget_set_halign(actionBox, GTK_ALIGN_CENTER);

    // gtk_widget_set_margin_top(actionBox, 10);
    // gtk_widget_set_margin_bottom(actionBox, 10);
    // gtk_widget_set_margin_start(actionBox, 10);
    // gtk_widget_set_margin_end(actionBox, 10);

    // gtk_frame_set_child(GTK_FRAME(actionFrame), actionBox);

    // gtk_widget_set_margin_top(actionFrame, 0);
    // gtk_widget_set_margin_bottom(actionFrame, 10);
    // gtk_widget_set_margin_start(actionFrame, 10);
    // gtk_widget_set_margin_end(actionFrame, 10);

    // /*
    // ** Encryption
    // */
    // encryptionFrame = gtk_frame_new("Encryption");
    // encryptionGrid = gtk_grid_new();

    // aesEncryptionRadio = gtk_check_button_new_with_label("AES");
    // xorEncryptionRadio = gtk_check_button_new_with_label("XOR");
    // noneEncryptionRadio = gtk_check_button_new_with_label("None");

    // gtk_check_button_set_group(GTK_CHECK_BUTTON(xorEncryptionRadio), GTK_CHECK_BUTTON(aesEncryptionRadio));
    // gtk_check_button_set_group(GTK_CHECK_BUTTON(noneEncryptionRadio), GTK_CHECK_BUTTON(aesEncryptionRadio));

    // aesPasswordField = gtk_password_entry_new();
    // gtk_widget_set_size_request(aesPasswordField, 100, 16);

    // xorKeystreamField = gtk_entry_new();
    // xorBrowseButton = gtk_button_new_with_label("Browse...");

    // gtk_widget_set_tooltip_text(xorBrowseButton, "Browse for a keystream file");

    // gtk_grid_attach(GTK_GRID(encryptionGrid), aesEncryptionRadio, 0, 0, 1, 1);
    // gtk_grid_attach(GTK_GRID(encryptionGrid), aesPasswordField, 1, 0, 2, 1);
    // gtk_grid_attach(GTK_GRID(encryptionGrid), xorEncryptionRadio, 0, 1, 1, 1);
    // gtk_grid_attach(GTK_GRID(encryptionGrid), xorKeystreamField, 1, 1, 1, 1);
    // gtk_grid_attach(GTK_GRID(encryptionGrid), xorBrowseButton, 2, 1, 1, 1);
    // gtk_grid_attach(GTK_GRID(encryptionGrid), noneEncryptionRadio, 0, 2, 1, 1);

    // gtk_grid_set_row_homogeneous(GTK_GRID(encryptionGrid), FALSE);
    // gtk_grid_set_row_spacing(GTK_GRID(encryptionGrid), 8);
    // gtk_grid_set_column_spacing(GTK_GRID(encryptionGrid), 10);
    
    // gtk_check_button_set_active(GTK_CHECK_BUTTON(aesEncryptionRadio), TRUE);

    // gtk_widget_set_margin_top(encryptionGrid, 10);
    // gtk_widget_set_margin_bottom(encryptionGrid, 10);
    // gtk_widget_set_margin_start(encryptionGrid, 10);
    // gtk_widget_set_margin_end(encryptionGrid, 10);

    // gtk_frame_set_child(GTK_FRAME(encryptionFrame), encryptionGrid);

    // gtk_widget_set_margin_top(encryptionFrame, 10);
    // gtk_widget_set_margin_bottom(encryptionFrame, 10);
    // gtk_widget_set_margin_start(encryptionFrame, 0);
    // gtk_widget_set_margin_end(encryptionFrame, 10);

    // /*
    // ** Quality
    // */
    // qualityFrame = gtk_frame_new("Quality");
    // qualityBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

    // highQualityRadio = gtk_check_button_new_with_label("High (1-bit)");
    // mediumQualityRadio = gtk_check_button_new_with_label("Medium (2-bits)");
    // lowQualityRadio = gtk_check_button_new_with_label("Low (4-bits)");

    // gtk_check_button_set_group(GTK_CHECK_BUTTON(mediumQualityRadio), GTK_CHECK_BUTTON(highQualityRadio));
    // gtk_check_button_set_group(GTK_CHECK_BUTTON(lowQualityRadio), GTK_CHECK_BUTTON(highQualityRadio));

    // gtk_check_button_set_active(GTK_CHECK_BUTTON(highQualityRadio), TRUE);

    // gtk_box_append(GTK_BOX(qualityBox), highQualityRadio);
    // gtk_box_append(GTK_BOX(qualityBox), mediumQualityRadio);
    // gtk_box_append(GTK_BOX(qualityBox), lowQualityRadio);

    // gtk_box_set_homogeneous(GTK_BOX(qualityBox), FALSE);

    // gtk_widget_set_margin_top(qualityBox, 10);
    // gtk_widget_set_margin_bottom(qualityBox, 10);
    // gtk_widget_set_margin_start(qualityBox, 10);
    // gtk_widget_set_margin_end(qualityBox, 10);

    // gtk_frame_set_child(GTK_FRAME(qualityFrame), qualityBox);

    // gtk_widget_set_margin_top(qualityFrame, 0);
    // gtk_widget_set_margin_bottom(qualityFrame, 10);
    // gtk_widget_set_margin_start(qualityFrame, 0);
    // gtk_widget_set_margin_end(qualityFrame, 10);

    // gtk_grid_attach(GTK_GRID(mainGrid), imageFrame, 0, 0, 1, 2);
    // gtk_grid_attach(GTK_GRID(mainGrid), actionFrame, 0, 2, 2, 1);
    // gtk_grid_attach(GTK_GRID(mainGrid), encryptionFrame, 1, 0, 1, 1);
    // gtk_grid_attach(GTK_GRID(mainGrid), qualityFrame, 1, 1, 1, 1);

    // gtk_window_set_child(GTK_WINDOW(mainWindow), mainGrid);

    // gtk_widget_set_halign(mainWindow, GTK_ALIGN_START);
    // gtk_widget_set_valign(mainWindow, GTK_ALIGN_CENTER);

    // gtk_widget_show(mainWindow);
}

int initiateGUI(int argc, char ** argv)
{
    GtkApplication *	app;
	int					status;

	app = gtk_application_new(APPLICATION_ID, G_APPLICATION_DEFAULT_FLAGS);

	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

	status = g_application_run(G_APPLICATION(app), argc, argv);

	g_object_unref(app);
    g_object_unref(_cloakInfo.builder);

	return status;
}
