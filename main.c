#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <unistd.h>
#define SQLITE_OPEN_READWRITE 0x00000002  /* Ok for sqlite3_open_v2() */

sqlite3 *db;
GtkWidget *window;
GtkWidget *statusbar;
guint context_id;
GtkListStore *liststore;

GError *error = NULL;

 typedef struct {
    GtkEntry *entry_titulo;
    GtkEntry *entry_autor;
    GtkEntry *entry_edition;
    GtkListStore *liststore;
			} EntryData;

// Callback function to process each row retrieved from the SELECT statement
int callback_select(void *data, int argc, char **argv, char **azColName) {
    int i;

    // Display the column names
    for (i = 0; i < argc; i++) {
        printf("%s: %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    
    printf("\n");
    
    return 0; // Return 0 to continue retrieving rows
}

void create_db(){
	
    char *zErrMsg = 0;
    int rc;
    char *dbPath;	
    asprintf(&dbPath, "%s/%s", DB_DIR, "books.db");

    // Open or creat a SQlite database
    rc = sqlite3_open_v2(dbPath, &db, SQLITE_OPEN_READWRITE, NULL);
    
    /*if (rc) {
        fprintf(stderr, "\nError opennig database: %s\n", sqlite3_errmsg(db));}
    else {
        fprintf(stderr, "\nDatabase successfully openned.\n");
    }*/

    // Creat the 'books' table if it doens't exist
    char *sql = "CREATE TABLE IF NOT EXISTS books (" \
                "ID INTEGER PRIMARY KEY AUTOINCREMENT," \
                "TITLE TEXT NOT NULL," \
                "AUTHOR TEXT NOT NULL," \
                "EDITION TEXT NOT NULL);";

    rc = sqlite3_exec(db, sql, callback_select, 0, &zErrMsg);
    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    } else {
        fprintf(stdout, "\nTable has successfully created.\n");}
        
free(dbPath);
}

void close_db() {
    if (db) {
        sqlite3_close(db);
      //  printf("\n\nDatabase successfully closed.\n");
      }
}

void add_books(GtkButton *button, gpointer user_data) {
    
    gtk_statusbar_push(GTK_STATUSBAR(statusbar), context_id, "");
    
    EntryData *livros = (EntryData *)user_data;

    const gchar *titulo_tmp = gtk_entry_get_text(livros->entry_titulo);
    const gchar *autor_tmp = gtk_entry_get_text(livros->entry_autor);
    const gchar *edition_tmp = gtk_entry_get_text(livros->entry_edition);
   
    char *tit = g_strdup(titulo_tmp);
	char *aut = g_strdup(autor_tmp);
	char *edit = g_strdup(edition_tmp);
	
	printf("%s",tit);
	
	if(strcmp(tit,"")==0){
		gtk_statusbar_push(GTK_STATUSBAR(statusbar), context_id, "Title could not be blank");
	}
	else{
		
    sqlite3_stmt *stmt;
    
    const char *query = "INSERT INTO books (TITLE, AUTHOR, EDITION) VALUES (?, ?, ?)";

    if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "\nFailed to prepare SQL statement: %s\n", sqlite3_errmsg(db));
        char *mens;
        asprintf(&mens, "%s %s", "Failed to prepare SQL statement: %s", sqlite3_errmsg(db));
        gtk_statusbar_push(GTK_STATUSBAR(statusbar), context_id, mens);
        return;
    }
    sqlite3_bind_text(stmt, 1, tit, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, aut, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, edit, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        fprintf(stderr, "\nFailed to execute SQL statement: %s\n", sqlite3_errmsg(db));
        char *mens1;
        asprintf(&mens1, "%s %s", "Failed to execute SQL statement: %s", sqlite3_errmsg(db));
        gtk_statusbar_push(GTK_STATUSBAR(statusbar), context_id, mens1);
    } else {
        fprintf(stdout, "\n\nBook successfully added!\n");
        gtk_statusbar_push(GTK_STATUSBAR(statusbar), context_id, "Book successfully added!");
    }

    sqlite3_finalize(stmt);
     // clean entry field
     gtk_entry_set_text(GTK_ENTRY(livros->entry_titulo), "");
     gtk_entry_set_text(GTK_ENTRY(livros->entry_autor), "");
     gtk_entry_set_text(GTK_ENTRY(livros->entry_edition), "");}
}

void load_books_into_treeview(GtkButton *button, GtkTreeView *treeview, gpointer user_data) {
    
     gtk_statusbar_push(GTK_STATUSBAR(statusbar), context_id, "");
     liststore= gtk_list_store_new(4, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    
    if (db == NULL || liststore == NULL) {
        fprintf(stderr, "Invalid database connection or GtkListStore.\n");
        gtk_statusbar_push(GTK_STATUSBAR(statusbar), context_id, "Invalid database connection or GtkListStore.");
        return;
    }
            
    char *sql = "SELECT ID, TITLE, AUTHOR, EDITION FROM books;";
    sqlite3_stmt *stmt;
    int rc;

    // Prepare the SQL statement
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare SQL statement: %s\n", sqlite3_errmsg(db));
        gtk_statusbar_push(GTK_STATUSBAR(statusbar), context_id, "Invalid database connection or GtkListStore.");
        return;
    }

    // Clean the GtkListStore for new values
    gtk_list_store_clear(liststore);

    // Loop for read each search result line
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        const unsigned int id = sqlite3_column_int(stmt, 0);
        const unsigned char *title = sqlite3_column_text(stmt, 1);
        const unsigned char *author = sqlite3_column_text(stmt, 2);
        const unsigned char *edition = sqlite3_column_text(stmt, 3);
        
        // Put the results in GtkListStore list
        GtkTreeIter iter;
        gtk_list_store_append(liststore, &iter);
        gtk_list_store_set(liststore, &iter, 0, id, 1, title, 2, author, 3, edition, -1);
    }

 // Conects the GtkListStore to the GtkTreeView
    gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), GTK_TREE_MODEL(liststore));

    // Unref the model
    g_object_unref(liststore);

    if (rc != SQLITE_DONE) {
        fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
        char *mens1;
        asprintf(&mens1, "%s %s", "SQL error:  ", sqlite3_errmsg(db));
        gtk_statusbar_push(GTK_STATUSBAR(statusbar), context_id, mens1);}
	
    // Ends the statement
    sqlite3_finalize(stmt);
}

void delete_books(GtkButton *button, GtkTreeView *treeview, gpointer data) {
    gtk_statusbar_push(GTK_STATUSBAR(statusbar), context_id, "");
    
    GtkWidget *dialog = gtk_dialog_new_with_buttons("", GTK_WINDOW(window),
                                                     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                     "Cancel", GTK_RESPONSE_CANCEL,
                                                     "Delete", GTK_RESPONSE_OK,
                                                     NULL);
                                                     
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget  *label_del = gtk_label_new ("Delete all?");
    gtk_container_add (GTK_CONTAINER (content_area), label_del);
    gtk_widget_show_all(dialog);
    gint response = gtk_dialog_run(GTK_DIALOG(dialog));

	    // if input was Delete, continue
    if (response == GTK_RESPONSE_OK) {
    GtkTreeModel *model;

	char *sql = "DELETE FROM books;";
    sqlite3_stmt *stmt;
    int rc;

    // Prepare the SQL statement
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare SQL statement: %s\n", sqlite3_errmsg(db));
        char *mens;
        asprintf(&mens, "%s %s", "Failed to prepare SQL statement: %s", sqlite3_errmsg(db));
        gtk_statusbar_push(GTK_STATUSBAR(statusbar), context_id, mens);
        return;
    }

	rc = sqlite3_exec(db, sql, NULL, NULL, NULL);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "Failed to execute DELETE statement: %s\n", sqlite3_errmsg(db));
		char *mens1;
        asprintf(&mens1, "%s %s", "Failed to execute DELETE statement: %s", sqlite3_errmsg(db));
        gtk_statusbar_push(GTK_STATUSBAR(statusbar), context_id, mens1);
}

	// Gets the current model link to the GtkTreeView
    model = gtk_tree_view_get_model(treeview);

    // If the model is a GtkListStore model, clean it up
    if (GTK_IS_LIST_STORE(model)) {
        liststore = GTK_LIST_STORE(model);
        gtk_list_store_clear(liststore);
    }
    
    // Ends the statement
    sqlite3_finalize(stmt);
    gtk_widget_destroy(dialog);}
    else{
    gtk_widget_destroy(dialog);}
}

void update_tree_view(GtkTreeView *treeview) {
    gtk_statusbar_push(GTK_STATUSBAR(statusbar), context_id, "");
    // Limpe o modelo atual
    gtk_list_store_clear(liststore);
    
    liststore = gtk_list_store_new(4, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
           
	char *sql = "SELECT ID, TITLE, AUTHOR, EDITION FROM books;";
	sqlite3_stmt *stmt;
	sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	
	// Clean the GtkListStore for new values
    gtk_list_store_clear(liststore);
	
	while ((sqlite3_step(stmt)) == SQLITE_ROW) {
				const unsigned int id = sqlite3_column_int(stmt, 0);
				const unsigned char *title = sqlite3_column_text(stmt, 1);
				const unsigned char *author = sqlite3_column_text(stmt, 2);
				const unsigned char *edition = sqlite3_column_text(stmt, 3);
				GtkTreeIter iter;
				gtk_list_store_append(liststore, &iter);
				gtk_list_store_set(liststore, &iter, 0, id, 1, title, 2, author, 3, edition, -1);
				}
			gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), GTK_TREE_MODEL(liststore));
			g_object_unref(liststore);
			sqlite3_finalize(stmt);
}

// Deletes data information from one ID input
void delete_records(GtkButton *button, GtkTreeView *treeview, gpointer data) {
   gtk_statusbar_push(GTK_STATUSBAR(statusbar), context_id, "");
   
   
    GtkWidget *dialog = gtk_dialog_new_with_buttons("", GTK_WINDOW(window),                                                    
                                                     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                     "Cancel", GTK_RESPONSE_CANCEL,
                                                     "Delete", GTK_RESPONSE_OK,
                                                     NULL);
		
		
		// Field for ID to be removed
		 
		GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
		GtkWidget *id_entry = gtk_entry_new();
		gtk_container_add(GTK_CONTAINER(content_area), id_entry);  
        GtkWidget  *label = gtk_label_new ("Enter the ID to be removed");
        gtk_container_add (GTK_CONTAINER (content_area), label);
	     // Show the dialog box and wait for user action
    gtk_widget_show_all(dialog);
	gint response = gtk_dialog_run(GTK_DIALOG(dialog));
	
    // If the answer was Remove, continue 
    if (response == GTK_RESPONSE_OK) {
	const gchar *id_text = gtk_entry_get_text(GTK_ENTRY(id_entry));

    char *sql;
    asprintf(&sql, "%s %s", "DELETE FROM books WHERE ID=", id_text);
	gtk_widget_destroy(dialog);
    // SQL exclusion
    char *err_message = NULL;
    int rc = sqlite3_exec(db, sql, NULL, NULL, &err_message);
    if (rc != SQLITE_OK) {
        GtkWidget *dialog1 = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "Error in remove entry: %s", err_message);
        gtk_dialog_run(GTK_DIALOG(dialog1));
        sqlite3_free(err_message);
        gtk_widget_destroy(dialog1);
    } else {
        // Exclusion done
        GtkWidget *dialog2 = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "Entry successfully removed");
        gint response2 = gtk_dialog_run(GTK_DIALOG(dialog2));
        
        if (response2 == GTK_RESPONSE_OK) {
			update_tree_view(treeview);
			 gtk_widget_destroy(dialog2);}
	
    }
        // clean entry field
       //gtk_entry_set_text(GTK_ENTRY(id_entry), "");
    }
    else{
	gtk_widget_destroy(dialog);
	}
}

void add_fig(GtkWidget *window){
    char *dbPath;
    asprintf(&dbPath, "%s/%s", fig_DIR, "books.png");
    /*if (!gtk_window_set_icon_from_file (GTK_WINDOW(window), dbPath, &error)) {
        g_error("Error loading icone: %s", error->message);
    }*/
    return;
free(dbPath);
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    // Build a main window
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Book Manager");
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    gtk_window_set_default_size(GTK_WINDOW(window), 910, 485);
    
	add_fig(window);
	
	create_db();
	
    // Build a vertical box to manage widgets
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_add(GTK_CONTAINER(window), vbox);
	
	GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	
    // Cria entradas para título e autor
    GtkWidget *entry_titulo = gtk_entry_new();
    GtkWidget *entry_autor = gtk_entry_new();
    GtkWidget *entry_edition = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_titulo), "Title");
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_autor), "Author");
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_edition), "Edition e.g. 1.ed");

    // Cria um botão
    GtkWidget *button_2 = gtk_button_new_with_label("Add");
    GtkWidget *button_3 = gtk_button_new_with_label("View");
    GtkWidget *button_4 = gtk_button_new_with_label("Delete All");
    GtkWidget *button_5 = gtk_button_new_with_label("Delete One");
    GtkWidget *button_6 = gtk_button_new_with_mnemonic ("_Exit");
    	
	/* Connect the button to the clicked signal. The callback function recieves the
	* window followed by the button because the arguments are swapped. */
	
	g_signal_connect_swapped(GTK_BUTTON(button_6), "clicked",G_CALLBACK (gtk_widget_destroy),(gpointer) window);

    gtk_box_pack_start(GTK_BOX(hbox), button_2, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), button_3, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), button_4, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), button_5, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), button_6, TRUE, TRUE, 0);
	
    // Cria uma lista de livros usando um GtkListStore
    GtkWidget *treeview;
    liststore = gtk_list_store_new(4, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(liststore));
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scrolled_window), treeview);

    // Configura as colunas da lista
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("ID", renderer, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
    
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Title", renderer, "text", 1, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Author", renderer, "text", 2, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
	
	renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Edition", renderer, "text", 3, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
	
	EntryData *data = g_malloc(sizeof(EntryData));
	
	data->entry_titulo = GTK_ENTRY(entry_titulo);
	data->entry_autor = GTK_ENTRY(entry_autor);
	data->entry_edition = GTK_ENTRY(entry_edition);
	data->liststore = liststore;
	
	g_signal_connect(button_2, "clicked", G_CALLBACK(add_books), data);
	g_signal_connect(button_3, "clicked", G_CALLBACK(load_books_into_treeview), treeview);
	g_signal_connect(button_4, "clicked", G_CALLBACK(delete_books), treeview);
	g_signal_connect(button_5, "clicked", G_CALLBACK(delete_records), treeview);
		
    // Adiciona os widgets à caixa vertical
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), entry_titulo, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), entry_autor, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), entry_edition, FALSE, FALSE, 0);
  
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);

    /* Connect the main window to the destroy and delete-event signals. */
	g_signal_connect(window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

	statusbar = gtk_statusbar_new();
    gtk_box_pack_end(GTK_BOX(vbox), statusbar, FALSE, TRUE, 0);
    context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar), "Statusbar Example");
	
    // Exibe todos os widgets
    gtk_widget_show_all(window);

    // Inicia o loop principal do GTK
    gtk_main();
	
	close_db();
    g_free(data);

    return 0;
}
