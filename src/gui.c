#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <gtk/gtk.h>

#include "cloak.h"
#include "cloak_types.h"
#include "utils.h"

static void activate(GtkApplication * app, gpointer user_data)
{
    GtkWidget *         mainWindow;
    GtkWidget *         mainGrid;
    GtkWidget *         image;
    GtkWidget *         capacityLabel;
    GtkWidget *         imageBox;
    GtkWidget *         actionBox;
    GtkWidget *         imageFrame;
    GtkWidget *         actionFrame;
    GtkWidget *         mergeButton;
    GtkWidget *         extractButton;
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

    mainWindow = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(mainWindow), "Cloak");

    mainGrid = gtk_grid_new();

    imageFrame = gtk_frame_new(NULL);
    imageBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

    pixbuf = gdk_pixbuf_new_from_file_at_size("/Users/guy/development/cloak/test/album.bmp", 500, 500, NULL);
    image = gtk_image_new_from_pixbuf(pixbuf);

    gtk_widget_set_size_request(image, 400, 400);

    capacityLabel = gtk_label_new_with_mnemonic("_Capacity: ");
    gtk_widget_set_halign(capacityLabel, GTK_ALIGN_START);

    gtk_box_append(GTK_BOX(imageBox), image);
    gtk_box_append(GTK_BOX(imageBox), capacityLabel);

    gtk_box_set_homogeneous(GTK_BOX(imageBox), FALSE);

    gtk_widget_set_margin_top(imageBox, 10);
    gtk_widget_set_margin_bottom(imageBox, 10);
    gtk_widget_set_margin_start(imageBox, 10);
    gtk_widget_set_margin_end(imageBox, 10);

    gtk_frame_set_child(GTK_FRAME(imageFrame), imageBox);

    gtk_widget_set_margin_top(imageFrame, 10);
    gtk_widget_set_margin_bottom(imageFrame, 10);
    gtk_widget_set_margin_start(imageFrame, 10);
    gtk_widget_set_margin_end(imageFrame, 10);

    actionFrame = gtk_frame_new(NULL);
    actionBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

    mergeButton = gtk_button_new_with_mnemonic("_Merge...");
    extractButton = gtk_button_new_with_mnemonic("_Extract...");

    gtk_button_set_use_underline(GTK_BUTTON(mergeButton), TRUE);
    gtk_button_set_use_underline(GTK_BUTTON(extractButton), TRUE);

    gtk_widget_set_tooltip_text(mergeButton, "Merge a secret file with the image");
    gtk_widget_set_tooltip_text(extractButton, "Extract a secret file from the image");

    gtk_box_append(GTK_BOX(actionBox), mergeButton);
    gtk_box_append(GTK_BOX(actionBox), extractButton);

    gtk_box_set_homogeneous(GTK_BOX(actionBox), FALSE);

    gtk_widget_set_halign(actionBox, GTK_ALIGN_CENTER);

    gtk_widget_set_margin_top(actionBox, 10);
    gtk_widget_set_margin_bottom(actionBox, 10);
    gtk_widget_set_margin_start(actionBox, 10);
    gtk_widget_set_margin_end(actionBox, 10);

    gtk_frame_set_child(GTK_FRAME(actionFrame), actionBox);

    gtk_widget_set_margin_top(actionFrame, 0);
    gtk_widget_set_margin_bottom(actionFrame, 10);
    gtk_widget_set_margin_start(actionFrame, 10);
    gtk_widget_set_margin_end(actionFrame, 10);

    /*
    ** Encryption
    */
    encryptionFrame = gtk_frame_new("Encryption");
    encryptionGrid = gtk_grid_new();

    aesEncryptionRadio = gtk_check_button_new_with_label("AES");
    xorEncryptionRadio = gtk_check_button_new_with_label("XOR");
    noneEncryptionRadio = gtk_check_button_new_with_label("None");

    gtk_check_button_set_group(GTK_CHECK_BUTTON(xorEncryptionRadio), GTK_CHECK_BUTTON(aesEncryptionRadio));
    gtk_check_button_set_group(GTK_CHECK_BUTTON(noneEncryptionRadio), GTK_CHECK_BUTTON(aesEncryptionRadio));

    aesPasswordField = gtk_password_entry_new();
    gtk_widget_set_size_request(aesPasswordField, 100, 16);

    xorKeystreamField = gtk_entry_new();
    xorBrowseButton = gtk_button_new_with_label("Browse...");

    gtk_widget_set_tooltip_text(xorBrowseButton, "Browse for a keystream file");

    gtk_grid_attach(GTK_GRID(encryptionGrid), aesEncryptionRadio, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(encryptionGrid), aesPasswordField, 1, 0, 2, 1);
    gtk_grid_attach(GTK_GRID(encryptionGrid), xorEncryptionRadio, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(encryptionGrid), xorKeystreamField, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(encryptionGrid), xorBrowseButton, 2, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(encryptionGrid), noneEncryptionRadio, 0, 2, 1, 1);

    gtk_grid_set_row_homogeneous(GTK_GRID(encryptionGrid), FALSE);
    gtk_grid_set_row_spacing(GTK_GRID(encryptionGrid), 8);
    gtk_grid_set_column_spacing(GTK_GRID(encryptionGrid), 10);
    
    gtk_check_button_set_active(GTK_CHECK_BUTTON(aesEncryptionRadio), TRUE);

    gtk_widget_set_margin_top(encryptionGrid, 10);
    gtk_widget_set_margin_bottom(encryptionGrid, 10);
    gtk_widget_set_margin_start(encryptionGrid, 10);
    gtk_widget_set_margin_end(encryptionGrid, 10);

    gtk_frame_set_child(GTK_FRAME(encryptionFrame), encryptionGrid);

    gtk_widget_set_margin_top(encryptionFrame, 10);
    gtk_widget_set_margin_bottom(encryptionFrame, 10);
    gtk_widget_set_margin_start(encryptionFrame, 0);
    gtk_widget_set_margin_end(encryptionFrame, 10);

    /*
    ** Quality
    */
    qualityFrame = gtk_frame_new("Quality");
    qualityBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

    highQualityRadio = gtk_check_button_new_with_label("High (1-bit)");
    mediumQualityRadio = gtk_check_button_new_with_label("Medium (2-bits)");
    lowQualityRadio = gtk_check_button_new_with_label("Low (4-bits)");

    gtk_check_button_set_group(GTK_CHECK_BUTTON(mediumQualityRadio), GTK_CHECK_BUTTON(highQualityRadio));
    gtk_check_button_set_group(GTK_CHECK_BUTTON(lowQualityRadio), GTK_CHECK_BUTTON(highQualityRadio));

    gtk_check_button_set_active(GTK_CHECK_BUTTON(highQualityRadio), TRUE);

    gtk_box_append(GTK_BOX(qualityBox), highQualityRadio);
    gtk_box_append(GTK_BOX(qualityBox), mediumQualityRadio);
    gtk_box_append(GTK_BOX(qualityBox), lowQualityRadio);

    gtk_box_set_homogeneous(GTK_BOX(qualityBox), FALSE);

    gtk_widget_set_margin_top(qualityBox, 10);
    gtk_widget_set_margin_bottom(qualityBox, 10);
    gtk_widget_set_margin_start(qualityBox, 10);
    gtk_widget_set_margin_end(qualityBox, 10);

    gtk_frame_set_child(GTK_FRAME(qualityFrame), qualityBox);

    gtk_widget_set_margin_top(qualityFrame, 0);
    gtk_widget_set_margin_bottom(qualityFrame, 10);
    gtk_widget_set_margin_start(qualityFrame, 0);
    gtk_widget_set_margin_end(qualityFrame, 10);

    gtk_grid_attach(GTK_GRID(mainGrid), imageFrame, 0, 0, 1, 2);
    gtk_grid_attach(GTK_GRID(mainGrid), actionFrame, 0, 2, 2, 1);
    gtk_grid_attach(GTK_GRID(mainGrid), encryptionFrame, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(mainGrid), qualityFrame, 1, 1, 1, 1);

    gtk_window_set_child(GTK_WINDOW(mainWindow), mainGrid);

    gtk_widget_set_halign(mainWindow, GTK_ALIGN_START);
    gtk_widget_set_valign(mainWindow, GTK_ALIGN_CENTER);

    gtk_widget_show(mainWindow);
}

int initiateGUI(int argc, char ** argv)
{
    GtkApplication *	app;
	int					status;

	app = gtk_application_new("com.guy.cloak", G_APPLICATION_DEFAULT_FLAGS);

	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

	status = g_application_run(G_APPLICATION(app), argc, argv);

	g_object_unref(app);

	return status;
}
