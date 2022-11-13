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

    capacityLabel = (GtkWidget *)gtk_builder_get_object(_cloakInfo.builder, "capacityLabel");
    gtk_label_set_label(GTK_LABEL(capacityLabel), capacityText);
}

static void handleKeystreamOpen(GtkNativeDialog * dialog, int response)
{
    GFile *         file;

    if (response == GTK_RESPONSE_ACCEPT) {
        GtkFileChooser * chooser = GTK_FILE_CHOOSER(dialog);

        file = gtk_file_chooser_get_file(chooser);
        _cloakInfo.pszKeystreamFile = g_file_get_path(file);
    }

    g_object_unref(dialog);
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

        image = (GtkWidget *)gtk_builder_get_object(_cloakInfo.builder, "image");
        pixBuf = gdk_pixbuf_new_from_file(_cloakInfo.pszSourceImageFile, NULL);

        gtk_image_set_from_pixbuf(GTK_IMAGE(image), pixBuf);

        refreshCapacity();
    }

    g_object_unref(dialog);
}

static void handleMergeOpen(GtkNativeDialog * dialog, int response)
{
    GdkPixbuf *     pixBuf;
    GtkWidget *     image;
    GtkWidget *     aesPasswordField;
    GFile *         file;
    char *          pszSecretFilename;
    char            szOutputImage[512];
    const char *    pszPassword;
    uint8_t         key[64];
    uint32_t        keyLength = 0U;

    if (response == GTK_RESPONSE_ACCEPT) {
        GtkFileChooser * chooser = GTK_FILE_CHOOSER(dialog);

        file = gtk_file_chooser_get_file(chooser);
        pszSecretFilename = g_file_get_path(file);

        strcpy(szOutputImage, "cloak_out.");
        strncat(szOutputImage, getFileExtension(_cloakInfo.pszSourceImageFile), 512);

        if (_cloakInfo.algo == aes256) {
            aesPasswordField = (GtkWidget *)gtk_builder_get_object(_cloakInfo.builder, "aesPasswordField");
            pszPassword = gtk_editable_get_text(GTK_EDITABLE(aesPasswordField));

            keyLength = getKey(key, 64, pszPassword);
        }

        merge(
            _cloakInfo.pszSourceImageFile, 
            pszSecretFilename, 
            _cloakInfo.pszKeystreamFile, 
            szOutputImage, 
            _cloakInfo.quality, 
            _cloakInfo.algo,
            key,
            keyLength);

        free(_cloakInfo.pszSourceImageFile);

        image = (GtkWidget *)gtk_builder_get_object(_cloakInfo.builder, "image");
        pixBuf = gdk_pixbuf_new_from_file(szOutputImage, NULL);

        gtk_image_set_from_pixbuf(GTK_IMAGE(image), pixBuf);
    }

    g_object_unref(dialog);
}

static void handleExtractSave(GtkNativeDialog * dialog, int response)
{
    GtkWidget *     aesPasswordField;
    GFile *         file;
    char *          pszSecretFilename;
    const char *    pszPassword;
    uint8_t         key[64];
    uint32_t        keyLength = 0U;

    if (response == GTK_RESPONSE_ACCEPT) {
        GtkFileChooser * chooser = GTK_FILE_CHOOSER(dialog);

        file = gtk_file_chooser_get_file(chooser);
        pszSecretFilename = g_file_get_path(file);
        g_print("Got file %s\n", pszSecretFilename);

        if (_cloakInfo.algo == aes256) {
            aesPasswordField = (GtkWidget *)gtk_builder_get_object(_cloakInfo.builder, "aesPasswordField");
            pszPassword = gtk_editable_get_text(GTK_EDITABLE(aesPasswordField));

            keyLength = getKey(key, 64, pszPassword);
        }

        extract(
            _cloakInfo.pszSourceImageFile,
            _cloakInfo.pszKeystreamFile,
            pszSecretFilename,
            _cloakInfo.quality,
            _cloakInfo.algo,
            key,
            keyLength);
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
        aesPasswordField = (GtkWidget *)gtk_builder_get_object(_cloakInfo.builder, "aesPasswordField");
        xorKeystreamFileField = (GtkWidget *)gtk_builder_get_object(_cloakInfo.builder, "xorKeystreamField");
        xorBrowseButton = (GtkWidget *)gtk_builder_get_object(_cloakInfo.builder, "xorBrowseButton");

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
        aesPasswordField = (GtkWidget *)gtk_builder_get_object(_cloakInfo.builder, "aesPasswordField");
        xorKeystreamFileField = (GtkWidget *)gtk_builder_get_object(_cloakInfo.builder, "xorKeystreamField");
        xorBrowseButton = (GtkWidget *)gtk_builder_get_object(_cloakInfo.builder, "xorBrowseButton");

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
        aesPasswordField = (GtkWidget *)gtk_builder_get_object(_cloakInfo.builder, "aesPasswordField");
        xorKeystreamFileField = (GtkWidget *)gtk_builder_get_object(_cloakInfo.builder, "xorKeystreamField");
        xorBrowseButton = (GtkWidget *)gtk_builder_get_object(_cloakInfo.builder, "xorBrowseButton");

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

    mergeActionRadio = (GtkWidget *)gtk_builder_get_object(_cloakInfo.builder, "mergeActionRadio");
    extractActionRadio = (GtkWidget *)gtk_builder_get_object(_cloakInfo.builder, "extractActionRadio");

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
    GtkFileChooserNative *          openDialog;

    openDialog = gtk_file_chooser_native_new(
                        "Open a keystream file", 
                        (GtkWindow *)data, 
                        GTK_FILE_CHOOSER_ACTION_OPEN, 
                        "_Open",
                        "_Cancel");

    g_signal_connect(openDialog, "response", G_CALLBACK(handleKeystreamOpen), NULL);
    gtk_native_dialog_show(GTK_NATIVE_DIALOG(openDialog));
}

static void handleOpenButtonClick(GtkWidget * widget, gpointer data)
{
    GtkFileChooserNative *          openDialog;
    GtkFileFilter *                 imageFilter;

    imageFilter = gtk_file_filter_new();

    gtk_file_filter_add_suffix(imageFilter, "png");
    gtk_file_filter_add_suffix(imageFilter, "bmp");

    openDialog = gtk_file_chooser_native_new(
                        "Open an image file", 
                        (GtkWindow *)data, 
                        GTK_FILE_CHOOSER_ACTION_OPEN, 
                        "_Open",
                        "_Cancel");

    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(openDialog), imageFilter);

    g_signal_connect(openDialog, "response", G_CALLBACK(handleImageOpen), NULL);
    gtk_native_dialog_show(GTK_NATIVE_DIALOG(openDialog));
}

static gboolean handleImageDrop(
                    GtkDropTarget * target, 
                    const GValue * value,
                    double x,
                    double y,
                    gpointer data)
{
    GdkPixbuf *     pixBuf;
    GtkWidget *     image = GTK_WIDGET(data);
    GdkFileList *   fileList;
    GSList *        list;
    char *          filePath;

    if (G_VALUE_HOLDS(value, GDK_TYPE_FILE_LIST)) {
        fileList = g_value_get_boxed(value);
        list = gdk_file_list_get_files(fileList);

        filePath = g_file_get_path(list->data);

        if (
            strncmp(getFileExtension(filePath), "png", 3) == 0 || 
            strncmp(getFileExtension(filePath), "bmp", 3) == 0)
        {
            _cloakInfo.pszSourceImageFile = filePath;
        }
        else {
            g_print("File dropped '%s' is not supported\n", filePath);
            return FALSE;
        }

        pixBuf = gdk_pixbuf_new_from_file(_cloakInfo.pszSourceImageFile, NULL);

        gtk_image_set_from_pixbuf(GTK_IMAGE(image), pixBuf);

        refreshCapacity();
    }
    else {
        return FALSE;
    }

    return TRUE;
}

static void activate(GtkApplication * app, gpointer user_data)
{
    GtkBuilder *        builder;
    GtkWidget *         mainWindow;
    GtkWidget *         openButton;
    GtkWidget *         image;
    GtkWidget *         goButton;
    GtkWidget *         aesEncryptionRadio;
    GtkWidget *         xorEncryptionRadio;
    GtkWidget *         noneEncryptionRadio;
    GtkWidget *         xorBrowseButton;
    GtkWidget *         highQualityRadio;
    GtkWidget *         mediumQualityRadio;
    GtkWidget *         lowQualityRadio;
    GdkPixbuf *         pixbuf;
    GtkDropTarget *     dropTarget;

    builder = gtk_builder_new();
    gtk_builder_add_from_file(builder, "builder.ui", NULL);

    _cloakInfo.builder = builder;

    mainWindow = (GtkWidget *)gtk_builder_get_object(builder, "mainWindow");
    gtk_window_set_application(GTK_WINDOW(mainWindow), app);

    openButton = (GtkWidget *)gtk_builder_get_object(builder, "openButton");
    g_signal_connect(openButton, "clicked", G_CALLBACK(handleOpenButtonClick), NULL);

    goButton = (GtkWidget *)gtk_builder_get_object(builder, "goButton");
    g_signal_connect(goButton, "clicked", G_CALLBACK(handleGoButtonClick), NULL);

    xorBrowseButton = (GtkWidget *)gtk_builder_get_object(builder, "xorBrowseButton");
    g_signal_connect(xorBrowseButton, "clicked", G_CALLBACK(handleBrowseButtonClick), NULL);

    highQualityRadio = (GtkWidget *)gtk_builder_get_object(builder, "highQualityRadio");
    g_signal_connect(highQualityRadio, "toggled", G_CALLBACK(handleHighQualityToggle), NULL);
    _cloakInfo.quality = quality_high;

    mediumQualityRadio = (GtkWidget *)gtk_builder_get_object(builder, "mediumQualityRadio");
    g_signal_connect(mediumQualityRadio, "toggled", G_CALLBACK(handleMediumQualityToggle), NULL);

    lowQualityRadio = (GtkWidget *)gtk_builder_get_object(builder, "lowQualityRadio");
    g_signal_connect(lowQualityRadio, "toggled", G_CALLBACK(handleLowQualityToggle), NULL);

    aesEncryptionRadio = (GtkWidget *)gtk_builder_get_object(builder, "aesEncryptionRadio");
    g_signal_connect(aesEncryptionRadio, "toggled", G_CALLBACK(handleAesEncryptionToggle), NULL);
    _cloakInfo.algo = aes256;

    xorEncryptionRadio = (GtkWidget *)gtk_builder_get_object(builder, "xorEncryptionRadio");
    g_signal_connect(xorEncryptionRadio, "toggled", G_CALLBACK(handleXorEncryptionToggle), NULL);

    noneEncryptionRadio = (GtkWidget *)gtk_builder_get_object(builder, "noneEncryptionRadio");
    g_signal_connect(noneEncryptionRadio, "toggled", G_CALLBACK(handleNoneEncryptionToggle), NULL);

    pixbuf = gdk_pixbuf_new_from_file_at_size("./initialImage.png", 400, 400, NULL);
    image = (GtkWidget *)gtk_builder_get_object(builder, "image");
    gtk_image_set_from_pixbuf(GTK_IMAGE(image), pixbuf);
    gtk_widget_set_size_request(image, 400, 400);

    dropTarget = gtk_drop_target_new(GDK_TYPE_FILE_LIST, GDK_ACTION_COPY);

    g_signal_connect(dropTarget, "drop", G_CALLBACK(handleImageDrop), image);
    gtk_widget_add_controller(GTK_WIDGET(image), GTK_EVENT_CONTROLLER(dropTarget));

    gtk_widget_show(mainWindow);
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
