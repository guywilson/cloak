/******************************************************************************
Copyright (c) 2023 Guy Wilson

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
******************************************************************************/#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>

#ifdef BUILD_GUI
#include <gtk/gtk.h>
#endif

#include "cloak.h"
#include "cloak_types.h"
#include "utils.h"

#ifdef BUILD_GUI
#define APPLICATION_ID "com.guy.cloak.cloak"

typedef enum {
    actionMerge,
    actionExtract
}
cloak_action;

typedef enum {
    modeImageDrop,
    modeFileDrop
}
drop_mode;

typedef struct {
    GtkBuilder *        builder;

    cloak_action        action;

    char *              pszSourceImageFile;
    char *              pszSourceSecretFile;
    char *              pszOutputFile;
    char *              pszKeystreamFile;

    merge_quality       quality;
    encryption_algo     algo;
}
CLOAK_INFO;

static CLOAK_INFO       _cloakInfo;


static void refreshCapacity() {
    GtkWidget *     capacityLabel;
    uint32_t        imageCapacity;
    char            capacityText[64];

    imageCapacity = (getImageCapacity(_cloakInfo.pszSourceImageFile, _cloakInfo.quality) / 1024);

    sprintf(capacityText, "Capacity: %u Kb", imageCapacity);

    capacityLabel = (GtkWidget *)gtk_builder_get_object(_cloakInfo.builder, "capacityLabel");
    gtk_label_set_label(GTK_LABEL(capacityLabel), capacityText);
}

static void onImageLoad() {
    static const char * fieldsToEnable[] = {
        "aesEncryptionRadio",
        "aesPasswordField",
        "xorEncryptionRadio",
        "noneEncryptionRadio",
        "highQualityRadio",
        "mediumQualityRadio",
        "lowQualityRadio",
        "mergeLabel",
        "extractLabel",
        "actionSwitch",
        "addFileButton",
        "N/A"
    };

    int             i = 0;
    GtkWidget *     widget;

    while (strcmp(fieldsToEnable[i], "N/A") != 0) {
        widget = (GtkWidget *)gtk_builder_get_object(_cloakInfo.builder, fieldsToEnable[i]);

        gtk_widget_set_sensitive(widget, TRUE);
        i++;
    }
}

static void handleKeystreamOpen(GtkNativeDialog * dialog, int response) {
    GFile *         file;
    GtkWidget *     xorKeystreamField;

    if (response == GTK_RESPONSE_ACCEPT) {
        GtkFileChooser * chooser = GTK_FILE_CHOOSER(dialog);

        file = gtk_file_chooser_get_file(chooser);
        _cloakInfo.pszKeystreamFile = g_file_get_path(file);

        xorKeystreamField = (GtkWidget *)gtk_builder_get_object(_cloakInfo.builder, "xorKeystreamField");
        gtk_editable_set_text(GTK_EDITABLE(xorKeystreamField), _cloakInfo.pszKeystreamFile);
    }

    gtk_native_dialog_destroy(dialog);
    g_object_unref(dialog);
}

static void handleImageOpen(GtkNativeDialog * dialog, int response) {
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

        g_object_unref(pixBuf);

        onImageLoad();

        refreshCapacity();
    }

    gtk_native_dialog_destroy(dialog);
    g_object_unref(dialog);
}

static gboolean handleFileDrop(
                    GtkDropTarget * target, 
                    const GValue * value,
                    double x,
                    double y,
                    gpointer data)
{
    static drop_mode    mode = modeImageDrop;
    GdkPixbuf *         pixBuf;
    GtkWidget *         image = GTK_WIDGET(data);
    GtkWidget *         goLabel;
    GtkWidget *         goButton;
    GtkWidget *         xorGenerateButton;
    GdkFileList *       fileList;
    GSList *            list;
    char *              filePath;
    char                szAction[256];

    if (G_VALUE_HOLDS(value, GDK_TYPE_FILE_LIST)) {
        fileList = g_value_get_boxed(value);
        list = gdk_file_list_get_files(fileList);

        filePath = g_file_get_path(list->data);

        if (mode == modeImageDrop) {
            if (
                strncmp(getFileExtension(filePath), "png", 3) == 0 || 
                strncmp(getFileExtension(filePath), "bmp", 3) == 0)
            {
                _cloakInfo.pszSourceImageFile = filePath;
                mode = modeFileDrop;
            }
            else {
                g_print("File dropped '%s' is not supported\n", filePath);
                return FALSE;
            }
        }
        else {
            if (
                strncmp(getFileExtension(filePath), "png", 3) == 0 || 
                strncmp(getFileExtension(filePath), "bmp", 3) == 0)
            {
                _cloakInfo.pszSourceImageFile = filePath;
                mode = modeFileDrop;
            }
            else {
                _cloakInfo.pszSourceSecretFile = filePath;

                sprintf(szAction, "Adding file '%s' to image", filePath);

                goLabel = (GtkWidget *)gtk_builder_get_object(_cloakInfo.builder, "goLabel");
                gtk_label_set_label(GTK_LABEL(goLabel), szAction);

                goButton = (GtkWidget *)gtk_builder_get_object(_cloakInfo.builder, "goButton");
                gtk_widget_set_sensitive(goButton, TRUE);

                xorGenerateButton = (GtkWidget *)gtk_builder_get_object(_cloakInfo.builder, "xorGenerateButton");
                gtk_widget_set_sensitive(xorGenerateButton, TRUE);

                mode = modeImageDrop;
                return TRUE;
            }
        }

        pixBuf = gdk_pixbuf_new_from_file(_cloakInfo.pszSourceImageFile, NULL);

        gtk_image_set_from_pixbuf(GTK_IMAGE(image), pixBuf);

        g_object_unref(pixBuf);

        onImageLoad();

        refreshCapacity();
    }
    else {
        return FALSE;
    }

    return TRUE;
}

static void handleMergeOpen(GtkNativeDialog * dialog, int response) {
    GFile *             file;
    GtkWidget *         goLabel;
    GtkWidget *         goButton;
    GtkWidget *         xorGenerateButton;
    char                szAction[256];

    if (response == GTK_RESPONSE_ACCEPT) {
        GtkFileChooser * chooser = GTK_FILE_CHOOSER(dialog);

        file = gtk_file_chooser_get_file(chooser);
        _cloakInfo.pszSourceSecretFile = g_file_get_path(file);

        sprintf(szAction, "Adding file '%s' to image", _cloakInfo.pszSourceSecretFile);

        goLabel = (GtkWidget *)gtk_builder_get_object(_cloakInfo.builder, "goLabel");
        gtk_label_set_label(GTK_LABEL(goLabel), szAction);

        goButton = (GtkWidget *)gtk_builder_get_object(_cloakInfo.builder, "goButton");
        gtk_widget_set_sensitive(goButton, TRUE);

        xorGenerateButton = (GtkWidget *)gtk_builder_get_object(_cloakInfo.builder, "xorGenerateButton");
        gtk_widget_set_sensitive(xorGenerateButton, TRUE);
    }

    gtk_native_dialog_destroy(dialog);
    g_object_unref(dialog);
}

static void handleExtractSave(GtkNativeDialog * dialog, int response) {
    GFile *             file;
    GtkWidget *         goLabel;
    GtkWidget *         goButton;
    char                szAction[256];

    if (response == GTK_RESPONSE_ACCEPT) {
        GtkFileChooser * chooser = GTK_FILE_CHOOSER(dialog);

        file = gtk_file_chooser_get_file(chooser);
        _cloakInfo.pszOutputFile = g_file_get_path(file);

        sprintf(szAction, "Extracting file '%s' from image", _cloakInfo.pszOutputFile);

        goLabel = (GtkWidget *)gtk_builder_get_object(_cloakInfo.builder, "goLabel");
        gtk_label_set_label(GTK_LABEL(goLabel), szAction);

        goButton = (GtkWidget *)gtk_builder_get_object(_cloakInfo.builder, "goButton");
        gtk_widget_set_sensitive(goButton, TRUE);
    }

    gtk_native_dialog_destroy(dialog);
    g_object_unref(dialog);
}

static void handleGenerateSave(GtkNativeDialog * dialog, int response) {
    GtkWidget *         xorKeystreamField;
    GFile *             file;
    uint32_t            numBytes;

    if (response == GTK_RESPONSE_ACCEPT) {
        GtkFileChooser * chooser = GTK_FILE_CHOOSER(dialog);

        file = gtk_file_chooser_get_file(chooser);
        _cloakInfo.pszKeystreamFile = g_file_get_path(file);

        numBytes = getFileSizeByName(_cloakInfo.pszSourceSecretFile);

        generateKeystreamFile(_cloakInfo.pszKeystreamFile, numBytes);

        xorKeystreamField = (GtkWidget *)gtk_builder_get_object(_cloakInfo.builder, "xorKeystreamField");
        gtk_editable_set_text(GTK_EDITABLE(xorKeystreamField), _cloakInfo.pszKeystreamFile);
    }

    gtk_native_dialog_destroy(dialog);
    g_object_unref(dialog);
}

static void handleActionSwitchState(GtkWidget * widget, gboolean state, gpointer data) {
    GtkWidget *         addFileButton;
    GtkWidget *         extractFileButton;
    GtkWidget *         goButton;

    addFileButton =     (GtkWidget *)gtk_builder_get_object(_cloakInfo.builder, "addFileButton");
    extractFileButton = (GtkWidget *)gtk_builder_get_object(_cloakInfo.builder, "extractFileButton");
    goButton =          (GtkWidget *)gtk_builder_get_object(_cloakInfo.builder, "goButton");

    if (state) {
        gtk_widget_set_sensitive(addFileButton, FALSE);
        gtk_widget_set_sensitive(extractFileButton, TRUE);
        gtk_button_set_label(GTK_BUTTON(goButton), "Extract");
        _cloakInfo.action = actionExtract;
    }
    else {
        gtk_widget_set_sensitive(addFileButton, TRUE);
        gtk_widget_set_sensitive(extractFileButton, FALSE);
        gtk_button_set_label(GTK_BUTTON(goButton), "Merge");
        _cloakInfo.action = actionMerge;
    }
}

static void handleAddFileButtonClick(GtkWidget * widget, gpointer data) {
    GtkFileChooserNative *          openDialog;

    openDialog = gtk_file_chooser_native_new(
                        "Open a secret file", 
                        (GtkWindow *)data, 
                        GTK_FILE_CHOOSER_ACTION_OPEN, 
                        "_Open",
                        "_Cancel");

    g_signal_connect(openDialog, "response", G_CALLBACK(handleMergeOpen), NULL);
    gtk_native_dialog_show(GTK_NATIVE_DIALOG(openDialog));
}

static void handleExtractFileButtonClick(GtkWidget * widget, gpointer data) {
    GtkFileChooserNative *          saveDialog;

    saveDialog = gtk_file_chooser_native_new(
                        "Save the secret file", 
                        (GtkWindow *)data, 
                        GTK_FILE_CHOOSER_ACTION_SAVE, 
                        "Save _As",
                        "_Cancel");

    g_signal_connect(saveDialog, "response", G_CALLBACK(handleExtractSave), NULL);
    gtk_native_dialog_show(GTK_NATIVE_DIALOG(saveDialog));
}

static void handleHighQualityToggle(GtkWidget * radio, gpointer data) {
    if (gtk_check_button_get_active(GTK_CHECK_BUTTON(radio))) {
        _cloakInfo.quality = quality_high;
        refreshCapacity();
    }
}

static void handleMediumQualityToggle(GtkWidget * radio, gpointer data) {
    if (gtk_check_button_get_active(GTK_CHECK_BUTTON(radio))) {
        _cloakInfo.quality = quality_medium;
        refreshCapacity();
    }
}

static void handleLowQualityToggle(GtkWidget * radio, gpointer data) {
    if (gtk_check_button_get_active(GTK_CHECK_BUTTON(radio))) {
        _cloakInfo.quality = quality_low;
        refreshCapacity();
    }
}

static void handleAesEncryptionToggle(GtkWidget * radio, gpointer data) {
    GtkWidget *     aesPasswordField;
    GtkWidget *     xorKeystreamFileField;
    GtkWidget *     xorBrowseButton;

    if (gtk_check_button_get_active(GTK_CHECK_BUTTON(radio))) {
        _cloakInfo.algo = aes256;

        aesPasswordField = (GtkWidget *)gtk_builder_get_object(_cloakInfo.builder, "aesPasswordField");
        xorKeystreamFileField = (GtkWidget *)gtk_builder_get_object(_cloakInfo.builder, "xorKeystreamField");
        xorBrowseButton = (GtkWidget *)gtk_builder_get_object(_cloakInfo.builder, "xorBrowseButton");

        gtk_widget_set_sensitive(aesPasswordField, TRUE);
        gtk_widget_set_sensitive(xorKeystreamFileField, FALSE);
        gtk_widget_set_sensitive(xorBrowseButton, FALSE);
    }
}

static void handleXorEncryptionToggle(GtkWidget * radio, gpointer data) {
    GtkWidget *     aesPasswordField;
    GtkWidget *     xorKeystreamFileField;
    GtkWidget *     xorBrowseButton;

    if (gtk_check_button_get_active(GTK_CHECK_BUTTON(radio))) {
        _cloakInfo.algo = xor;
        
        aesPasswordField = (GtkWidget *)gtk_builder_get_object(_cloakInfo.builder, "aesPasswordField");
        xorKeystreamFileField = (GtkWidget *)gtk_builder_get_object(_cloakInfo.builder, "xorKeystreamField");
        xorBrowseButton = (GtkWidget *)gtk_builder_get_object(_cloakInfo.builder, "xorBrowseButton");

        gtk_widget_set_sensitive(aesPasswordField, FALSE);
        gtk_widget_set_sensitive(xorKeystreamFileField, TRUE);
        gtk_widget_set_sensitive(xorBrowseButton, TRUE);
    }
}

static void handleNoneEncryptionToggle(GtkWidget * radio, gpointer data) {
    GtkWidget *     aesPasswordField;
    GtkWidget *     xorKeystreamFileField;
    GtkWidget *     xorBrowseButton;

    if (gtk_check_button_get_active(GTK_CHECK_BUTTON(radio))) {
        _cloakInfo.algo = none;
        
        aesPasswordField = (GtkWidget *)gtk_builder_get_object(_cloakInfo.builder, "aesPasswordField");
        xorKeystreamFileField = (GtkWidget *)gtk_builder_get_object(_cloakInfo.builder, "xorKeystreamField");
        xorBrowseButton = (GtkWidget *)gtk_builder_get_object(_cloakInfo.builder, "xorBrowseButton");

        gtk_widget_set_sensitive(aesPasswordField, FALSE);
        gtk_widget_set_sensitive(xorKeystreamFileField, FALSE);
        gtk_widget_set_sensitive(xorBrowseButton, FALSE);
    }
}

static void handleGoButtonClick(GtkWidget * widget, gpointer data) {
    GdkPixbuf *                     pixBuf;
    GtkWidget *                     image;
    GtkWidget *                     aesPasswordField;
    char                            szOutputImage[512];
    const char *                    pszPassword;
    uint8_t                         key[64];
    uint32_t                        keyLength = 0U;

    if (_cloakInfo.algo == aes256) {
        aesPasswordField = (GtkWidget *)gtk_builder_get_object(_cloakInfo.builder, "aesPasswordField");
        pszPassword = gtk_editable_get_text(GTK_EDITABLE(aesPasswordField));

        keyLength = getKey(key, 64, pszPassword);
    }

    if (_cloakInfo.action == actionMerge) {
        strcpy(szOutputImage, "cloak_out.");
        strncat(szOutputImage, getFileExtension(_cloakInfo.pszSourceImageFile), 512);

        merge(
            _cloakInfo.pszSourceImageFile, 
            _cloakInfo.pszSourceSecretFile, 
            _cloakInfo.pszKeystreamFile, 
            szOutputImage, 
            _cloakInfo.quality, 
            _cloakInfo.algo,
            key,
            keyLength);

        image = (GtkWidget *)gtk_builder_get_object(_cloakInfo.builder, "image");
        pixBuf = gdk_pixbuf_new_from_file(szOutputImage, NULL);

        gtk_image_set_from_pixbuf(GTK_IMAGE(image), pixBuf);

        g_object_unref(pixBuf);
    }
    else {
        extract(
            _cloakInfo.pszSourceImageFile,
            _cloakInfo.pszKeystreamFile,
            _cloakInfo.pszOutputFile,
            _cloakInfo.quality,
            _cloakInfo.algo,
            key,
            keyLength);
    }
}

static void handleCloseButtonClick(GtkWidget * widget, gpointer data) {
    GtkWidget *                     mainWindow;

    mainWindow = (GtkWidget *)gtk_builder_get_object(_cloakInfo.builder, "mainWindow");
    gtk_window_close(GTK_WINDOW(mainWindow));
    gtk_window_destroy(GTK_WINDOW(mainWindow));
}

static void handleBrowseButtonClick(GtkWidget * widget, gpointer data) {
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

static void handleGenerateButtonClick(GtkWidget * widget, gpointer data) {
    GtkFileChooserNative *          saveDialog;

    saveDialog = gtk_file_chooser_native_new(
                        "Save the OTP file", 
                        (GtkWindow *)data, 
                        GTK_FILE_CHOOSER_ACTION_SAVE, 
                        "Save _As",
                        "_Cancel");

    g_signal_connect(saveDialog, "response", G_CALLBACK(handleGenerateSave), NULL);
    gtk_native_dialog_show(GTK_NATIVE_DIALOG(saveDialog));
}

static void handleOpenButtonClick(GtkWidget * widget, gpointer data) {
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

static void activate(GtkApplication * app, gpointer user_data) {
    GtkBuilder *        builder;
    GdkPixbuf *         pixbuf;
    GtkDropTarget *     dropTarget;
    // GtkBuilder *        menuBuilder;
    // GMenuModel *        menuBar;
    GtkWidget *         mainWindow;
    GtkWidget *         openButton;
    GtkWidget *         image;
    GtkWidget *         goButton;
    GtkWidget *         closeButton;
    GtkWidget *         aesEncryptionRadio;
    GtkWidget *         xorEncryptionRadio;
    GtkWidget *         noneEncryptionRadio;
    GtkWidget *         xorBrowseButton;
    GtkWidget *         highQualityRadio;
    GtkWidget *         mediumQualityRadio;
    GtkWidget *         lowQualityRadio;
    GtkWidget *         actionSwitch;
    GtkWidget *         addFileButton;
    GtkWidget *         extractFileButton;
    GtkWidget *         xorGenerateButton;

    builder = gtk_builder_new_from_resource("/com/guy/cloak/resources/builder.ui");

    _cloakInfo.builder = builder;

    mainWindow = (GtkWidget *)gtk_builder_get_object(builder, "mainWindow");
    gtk_window_set_application(GTK_WINDOW(mainWindow), app);

    // menuBuilder = gtk_builder_new_from_resource("/com/guy/cloak/resources/menu.ui");
    // menuBar = G_MENU_MODEL(gtk_builder_get_object(menuBuilder, "menubar"));

    // gtk_application_set_menubar(app, menuBar);
    // g_object_unref(menuBuilder);

    openButton = (GtkWidget *)gtk_builder_get_object(builder, "openButton");
    g_signal_connect(openButton, "clicked", G_CALLBACK(handleOpenButtonClick), NULL);

    goButton = (GtkWidget *)gtk_builder_get_object(builder, "goButton");
    g_signal_connect(goButton, "clicked", G_CALLBACK(handleGoButtonClick), NULL);

    closeButton = (GtkWidget *)gtk_builder_get_object(builder, "closeButton");
    g_signal_connect(closeButton, "clicked", G_CALLBACK(handleCloseButtonClick), NULL);

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

    actionSwitch = (GtkWidget *)gtk_builder_get_object(builder, "actionSwitch");
    g_signal_connect(actionSwitch, "state-set", G_CALLBACK(handleActionSwitchState), NULL);
    _cloakInfo.action = actionMerge;

    addFileButton = (GtkWidget *)gtk_builder_get_object(builder, "addFileButton");
    g_signal_connect(addFileButton, "clicked", G_CALLBACK(handleAddFileButtonClick), NULL);

    extractFileButton = (GtkWidget *)gtk_builder_get_object(builder, "extractFileButton");
    g_signal_connect(extractFileButton, "clicked", G_CALLBACK(handleExtractFileButtonClick), NULL);

    xorGenerateButton = (GtkWidget *)gtk_builder_get_object(builder, "xorGenerateButton");
    g_signal_connect(xorGenerateButton, "clicked", G_CALLBACK(handleGenerateButtonClick), NULL);

    pixbuf = gdk_pixbuf_new_from_resource_at_scale("/com/guy/cloak/resources/initialimage.png", 400, 400, TRUE, NULL);
    image = (GtkWidget *)gtk_builder_get_object(builder, "image");
    gtk_image_set_from_pixbuf(GTK_IMAGE(image), pixbuf);
    gtk_widget_set_size_request(image, 400, 400);

    dropTarget = gtk_drop_target_new(GDK_TYPE_FILE_LIST, GDK_ACTION_COPY);

    g_signal_connect(dropTarget, "drop", G_CALLBACK(handleFileDrop), image);
    gtk_widget_add_controller(GTK_WIDGET(image), GTK_EVENT_CONTROLLER(dropTarget));

    gtk_widget_show(mainWindow);
}

int initiateGUI(int argc, char ** argv) {
    GtkApplication *	app;
	int					status;

	app = gtk_application_new(APPLICATION_ID, G_APPLICATION_DEFAULT_FLAGS);

	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

	status = g_application_run(G_APPLICATION(app), argc, argv);

	g_object_unref(app);
    g_object_unref(_cloakInfo.builder);

	return status;
}

#endif
