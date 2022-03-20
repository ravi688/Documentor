
#include <gtk/gtk.h>
#include <string.h>
#include <bufferlib/buffer.h>
#include "Serialization.h"
#include "XmlParser.h"
#include "defs.h"


typedef struct
{
	const char* label; 
	const char* header; 
	const char* content; 
} Page; 

static void notebook_add_page(GtkNotebook* chapters, Page* page); 
static void activate(GtkApplication* app, gpointer user_data); 
static void get_page_data_from_file(const char* file_name);
static Page* get_page_from_xml_tag(XMLtag* tag);


static Page* get_page_from_xml_tag(XMLtag* tag)
{
	#ifdef DEBUG
	puts("Creating Page");
	#endif
	Page* page = (Page*)instantiate_object("Page");
	BUFFER* previous_buffer = BUFget_binded_buffer();
	#ifdef DEBUG
	XMLtag_print(tag, 0);
	#endif
	if(tag->childs != NULL)
	{
		BUFbind(tag->childs);
		for(int i = 0; i < BUFget_element_count(); i++)
		{
			XMLtag* _tag = (XMLtag*)BUFgetptr_at(i); 
			SerializedProperty property = serialized_struct_get_property("Page", _tag->name, (void*)page);
			serialized_property_set_value(&property, (void*)((intptr_t)(&(_tag->content))));
			BUFbind(tag->childs);
		}
	}
	BUFbind(previous_buffer);
	#ifdef DEBUG
	puts("Page created successfully");
	#endif
	return page;
}

static BUFFER* get_page_data(BUFFER* xml_tags)
{
	BUFFER* pages = BUFcreate(NULL, sizeof(Page*), 1, 0);
	BUFbind(xml_tags); 
	XMLtag* _tag = (XMLtag*)BUFgetptr_at(0);
	for(int i = 0; i < BUFget_element_count(); i++)
	{
		Page* page = get_page_from_xml_tag((XMLtag*)BUFgetptr_at(i));
		BUFbind(pages); 
		BUFpush(&page); 
		BUFbind(xml_tags);
	}
	return pages;
}

PangoFontDescription* pango_font = NULL;
PangoFontDescription* tab_font = NULL;
PangoFontDescription* content_font = NULL;

static void notebook_add_page(GtkNotebook* chapters, Page* page)
{
	GtkWidget* horizontal_paned = gtk_paned_new(GTK_ORIENTATION_VERTICAL); 
	GtkWidget* header = gtk_label_new(page->header);

	if(pango_font == NULL)
	{
		pango_font = pango_font_description_from_string("Sans Bold 12");
		tab_font = pango_font_description_from_string("Consolas Bold 10");
		content_font = pango_font_description_from_string("Arial Bold 11");
		gtk_widget_override_font(GTK_WIDGET(chapters), tab_font);
	}
	gtk_widget_override_font(header, pango_font);

	GtkWidget* content = gtk_label_new(page->content);
	gtk_widget_override_font(content, content_font);

	gtk_label_set_line_wrap(GTK_LABEL(header), TRUE); 
	gtk_label_set_line_wrap(GTK_LABEL(content), TRUE);  

	gtk_paned_pack1(GTK_PANED(horizontal_paned), header, FALSE, FALSE); 
	gtk_paned_pack2(GTK_PANED(horizontal_paned), content, FALSE, FALSE); 
	gtk_paned_set_position(GTK_PANED(horizontal_paned), 60); 

	GtkWidget* label = gtk_label_new(page->label); 
	gtk_notebook_append_page(chapters, horizontal_paned, label); 
}

static void activate(GtkApplication* app, gpointer user_data)
{
	GtkWidget* window = gtk_application_window_new(app); 
	gtk_window_set_default_size(GTK_WIDGET(window), 4 * 200, 3 * 200); 
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_title(GTK_WINDOW(window), "Documentor"); 

	GtkWidget* chapters = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(chapters), GTK_POS_LEFT); 

	load_serialization_source_file(__FILE__);
	struct_serialize("Page");
	char* text_buffer = load_text_from_file_exclude_comments("./data/Data.xml");
	XMLdata xml_data = XMLparse(text_buffer);

	BUFFER* pages = get_page_data(xml_data.tags);

	BUFbind(pages);
	for(int i = 0; i < BUFget_element_count(); i++)
	{
		Page* page = *((Page**)BUFgetptr_at(i));
		notebook_add_page(GTK_NOTEBOOK(chapters), page);
	}

	gtk_container_add(GTK_CONTAINER(window), chapters);

	gtk_widget_show_all(window); 	

	XMLdata_destroy(xml_data);
	destroy_serialization_data();

	BUFbind(pages); 
	for(int i = 0; i < BUFget_element_count(); i++)
	{
		Page* page = *((Page**)BUFgetptr_at(i)); 
		free(page);
	}
	BUFfree(); 

	free(text_buffer);
}



int main(int argc, char** argv)
{
	GtkApplication* app = gtk_application_new("com.PhyMac.Documentor", G_APPLICATION_FLAGS_NONE); 

	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL); 

	int status = g_application_run(G_APPLICATION(app), argc, argv);

	g_object_unref(app); 

	return status;  
}