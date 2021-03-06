/*
	NEStopia / Linux
	Port by R. Belmont
	
	auxio.cpp - handles movie and state I/O
*/

#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "core/api/NstApiEmulator.hpp"
#include "core/api/NstApiVideo.hpp"
#include "core/api/NstApiSound.hpp"
#include "core/api/NstApiInput.hpp"
#include "core/api/NstApiMachine.hpp"
#include "core/api/NstApiUser.hpp"
#include "core/api/NstApiNsf.hpp"
#include "core/api/NstApiMovie.hpp"
#include "core/api/NstApiFds.hpp"
#include "core/api/NstApiCartridge.hpp"
#include "oss.h"
#include "settings.h"
#if 1 // AWH
#include "unzip/unzip.h" // AWH - Added unzip directory to path
#include "../../beagleboard.h" // AWH - Platform-specific paths
#include "../../gui.h" // AWH

extern char savedirname[1024]; /* From unix/main.cpp */
#endif // AWH

#if 0 // AWH
extern "C" {
// AWH #include <gtk/gtk.h>

#include "7zCrc.h"
#include "7zIn.h"
#include "7zExtract.h"
#include "7zAlloc.h"

// AWH #include "interface.h"
//#include "callbacks.h"
}
#endif // AWH

#define MAX_ITEMS	(512)

extern Nes::Api::Emulator emulator;
// AWH extern GtkWidget *mainwindow;
extern char rootname[512];

static std::ifstream *moviePlayFile, *fdsBiosFile, *nstDBFile;
static std::fstream *movieRecFile;

static bool run_picker, cancelled;
#if 0 // AWH
static GtkTreeStore *treestore;
static GtkTreeIter treeiters[MAX_ITEMS];
static GtkCellRenderer *renderer;
static GtkTreeViewColumn *column;
static GtkTreeSelection *selection;

static int find_current_selection(void)
{
	int i;

	for (i = 0; i < MAX_ITEMS; i++)
	{
		if (gtk_tree_selection_iter_is_selected(selection, &treeiters[i]))
		{
			return i;
		}
	}

	return -1;
}

void on_archok_clicked(GtkButton *button, gpointer user_data)
{
	run_picker = false;
}

void on_archcancel_clicked(GtkButton *button, gpointer user_data)
{
	run_picker = false;
	cancelled = true;
}

void on_archselect_destroyed(GtkButton *button, gpointer user_data)
{
	run_picker = false;
}

static gint check_list_double(GtkWidget *widget, GdkEventButton *event, gpointer func_data)
{
	if (event->type==GDK_2BUTTON_PRESS)
	{
		run_picker = false;
	}

	return FALSE;
}
#endif // AWH
void auxio_init(void)
{
	moviePlayFile = NULL;
	movieRecFile = NULL;
	fdsBiosFile = NULL;
	nstDBFile = NULL;
}

void auxio_do_state_save(void)
{
	Nes::Api::Machine machine( emulator );
	// AWH GtkWidget *dialog;
	char defname[512];

	defname[0] = '\0';
	strcpy(defname, rootname);
	strcat(defname, ".nst");
#if 0 // AWH
	dialog = gtk_file_chooser_dialog_new ("Save state (.nst)",
					      GTK_WINDOW(mainwindow),
					      GTK_FILE_CHOOSER_ACTION_SAVE,
					      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					      GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
					      NULL);

	gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), defname);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
	{
		char *filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
#else
		char *filename = defname;
#endif // AWH
		std::ofstream stateFile( filename, std::ifstream::out|std::ifstream::binary );
fprintf(stderr, "NES: Saving game state '%s'\n", filename);
		if (stateFile.is_open())
		{
			machine.SaveState(stateFile);
#if 1 // AWH - Sync the file in the save game directory
			int fd = open(filename, O_RDWR);
			fsync(fd);
			close(fd);
			fd = open(savedirname, O_RDWR);
			fsync(fd);
			close(fd);
#endif // AWH
		}
#if 0 // AWH
		g_free (filename);
	}

	gtk_widget_destroy(dialog);
#endif // AWH
}

void auxio_do_state_load(void)
{
	Nes::Api::Machine machine( emulator );
#if 0 // AWH
	GtkWidget *dialog;
	GtkFileFilter *filter;

	dialog = gtk_file_chooser_dialog_new ("Load state (.nst)",
					      GTK_WINDOW(mainwindow),
					      GTK_FILE_CHOOSER_ACTION_OPEN,
					      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					      GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
					      NULL);

	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "Nestopia save states");
	gtk_file_filter_add_pattern(filter, "*.nst");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
	{
		char *filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
#else
		char filename[1024];
		strcpy(filename, rootname);
		strcat(filename, ".nst");
		fprintf(stderr, "NES: Load '%s'\n", filename);
#endif // AWH
		std::ifstream stateFile( filename, std::ifstream::in|std::ifstream::binary );

		if (stateFile.is_open())
		{
			machine.LoadState(stateFile);
		}
#if 0 // AWH
		g_free (filename);
	}

	gtk_widget_destroy(dialog);
#endif // AWH
}
#if 0 // AWH
void auxio_do_movie_save(void)
{
	Nes::Api::Machine machine( emulator );
	Nes::Api::Movie movie( emulator );
	GtkWidget *dialog;
	char defname[512];

	defname[0] = '\0';
	strcpy(defname, rootname);
	strcat(defname, ".nsv");

	if (movieRecFile)
	{
		delete movieRecFile;
		movieRecFile = NULL;
	}

	dialog = gtk_file_chooser_dialog_new ("Save a movie (.nsv)",
					      GTK_WINDOW(mainwindow),
					      GTK_FILE_CHOOSER_ACTION_SAVE,
					      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					      GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
					      NULL);

	gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), defname);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
	{
		char *filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

		movieRecFile = new std::fstream(filename, std::ifstream::out|std::ifstream::binary); 

		if (movieRecFile->is_open())
		{
			movie.Record((std::iostream&)*movieRecFile, Nes::Api::Movie::CLEAN);
		}
		else
		{
			delete movieRecFile;
			movieRecFile = NULL;
		}

		g_free (filename);
	}

	gtk_widget_destroy(dialog);
}

void auxio_do_movie_load(void)
{
	Nes::Api::Machine machine( emulator );
	Nes::Api::Movie movie( emulator );
	GtkWidget *dialog;
	GtkFileFilter *filter;

	if (moviePlayFile)
	{
		delete moviePlayFile;
		moviePlayFile = NULL;
	}

	dialog = gtk_file_chooser_dialog_new ("Load a movie (.nsv)",
					      GTK_WINDOW(mainwindow),
					      GTK_FILE_CHOOSER_ACTION_OPEN,
					      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					      GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
					      NULL);

	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "Nestopia movies");
	gtk_file_filter_add_pattern(filter, "*.nsv");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
	{
		char *filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

		moviePlayFile = new std::ifstream( filename, std::ifstream::in|std::ifstream::binary ); 

		if (moviePlayFile->is_open())
		{
			movie.Play(*moviePlayFile);
		}
		else
		{
			delete moviePlayFile;
			moviePlayFile = NULL;
		}

		g_free (filename);
	}

	gtk_widget_destroy(dialog);
}

void auxio_do_movie_stop(void)
{
	Nes::Api::Movie movie( emulator );

	if (movieRecFile || moviePlayFile)
	{
		movie.Stop();
		movie.Eject();

		if (movieRecFile)
		{
			delete movieRecFile;
			movieRecFile = NULL;
		}

		if (moviePlayFile)
		{
			delete moviePlayFile;
			moviePlayFile = NULL;
		}
	}
}
#endif // AWH
void auxio_set_fds_bios(void)
{
	Nes::Api::Fds fds( emulator );
	char dirname[1024], *home;

	if (fdsBiosFile)
	{
		return;
	}
#if 0 // AWH
	home = getenv("HOME");
	sprintf(dirname, "%s/.nestopia/disksys.rom", home);
#else
	sprintf(dirname, "%s/disksys.rom", BES_FILE_ROOT_DIR); 
#endif // AWH
	fdsBiosFile = new std::ifstream(dirname, std::ifstream::in|std::ifstream::binary);

	if (fdsBiosFile->is_open())
	{
		fds.SetBIOS(fdsBiosFile);
	}
	else
	{
		std::cout << "Couldn't find disksys.rom\nDisk System games will not work\n";
		delete fdsBiosFile;
		fdsBiosFile = NULL;
	}
}

void auxio_shutdown(void)
{
	if (nstDBFile)
	{
		delete nstDBFile;
		nstDBFile = NULL;
	}

	if (fdsBiosFile)
	{
 		delete fdsBiosFile;
		fdsBiosFile = NULL;
	}
}

static int checkExtension(char *filename)
{
	int nlen;

	nlen = strlen(filename);

	if ((!strcasecmp(&filename[nlen-4], ".nes")) ||
	    (!strcasecmp(&filename[nlen-4], ".fds")) ||
	    (!strcasecmp(&filename[nlen-4], ".nsf")) ||
	    (!strcasecmp(&filename[nlen-4], ".unf")) ||
	    (!strcasecmp(&filename[nlen-5], ".unif"))||
	    (!strcasecmp(&filename[nlen-4], ".xml")))
	{
		return 1;
	}

	return 0;
}

typedef FILE *MY_FILE_HANDLE;

size_t MyReadFile(MY_FILE_HANDLE file, void *data, size_t size)
{ 
  if (size == 0)
    return 0;
  return fread(data, 1, size, file); 
}
#if 0 // AWH - 7zip
typedef struct _CFileInStream
{
  ISzInStream InStream;
  MY_FILE_HANDLE File;
} CFileInStream;

#define kBufferSize (1 << 12)
Byte g_Buffer[kBufferSize];

SRes SzFileReadImp(void *object, void **buffer, size_t *size)
{
  CFileInStream *s = (CFileInStream *)object;
  if (*size > kBufferSize)
    *size = kBufferSize;
  *size = MyReadFile(s->File, g_Buffer, *size);
  *buffer = g_Buffer;
  return SZ_OK;
}

SRes SzFileSeekImp(void *object, CFileSize pos, ESzSeek origin)
{
  CFileInStream *s = (CFileInStream *)object;

  int moveMethod;
  int res;
  switch (origin) 
  {
    case SZ_SEEK_SET: moveMethod = SEEK_SET; break;
    case SZ_SEEK_CUR: moveMethod = SEEK_CUR; break;
    case SZ_SEEK_END: moveMethod = SEEK_END; break;
    default: return SZ_ERROR_PARAM;
  }
  res = fseek(s->File, (long)pos, moveMethod );
  return (res == 0) ? SZ_OK : SZ_ERROR_FAIL;
}
#endif // AWH

int auxio_load_archive(const char *filename, unsigned char **dataout, int *datasize, int *dataoffset, const char *filetoload, char *outname)
{
	FILE *f;
	unsigned char idbuf[4];
	int filesFound = 0;
	std::vector<char *> filelist;	// list of files we can load in this archive

	// default case: outname is filename
	if (outname)
	{
		strcpy(outname, filename);
	}

	f = fopen(filename, "rb");

	if (!f)
	{
		return 0;	// no good
	}

	fread(idbuf, 4, 1, f);
	fclose(f);

//	printf("ID bytes %c %c %x %x\n", idbuf[0], idbuf[1], idbuf[2], idbuf[3]);

	if ((idbuf[0] == 'P') && (idbuf[1] == 'K') && (idbuf[2] == 0x03) && (idbuf[3] == 0x04))
	{	// it's zip
		unzFile zipArchive;
		char file_name[1024];
		unz_file_info info;
		int nlen;
		int ret = 0;

		zipArchive = unzOpen(filename);

		if (!zipArchive)
		{
			return 0;
		}

		unzGoToFirstFile(zipArchive);
		
		for (;;)
		{
			if (unzGetCurrentFileInfo(zipArchive, &info, file_name, 1024, NULL, 0, NULL, 0) == UNZ_OK)
			{
				if (filetoload != NULL)
				{
					if (!strcasecmp(filetoload, file_name))
					{
						int length;
						unsigned char *buffer;

						unzOpenCurrentFile(zipArchive);
					
						length = info.uncompressed_size;
						buffer = (unsigned char *)malloc(length);

				    		ret = unzReadCurrentFile(zipArchive, buffer, length);

						if (ret != length)
						{
							free(buffer);
							return 0;
						}

						unzCloseCurrentFile(zipArchive);
						unzClose(zipArchive);

						*datasize = length;
						*dataout = buffer;
						*dataoffset = 0;

						return 1;
					}
				}
				else
				{
					if (checkExtension(file_name))
					{
						char *tmpstr;

						tmpstr = (char *)malloc(strlen(file_name)+1);
						strcpy(tmpstr, file_name);

						// add to the file list
						filelist.push_back(tmpstr);
						filesFound++;
					}
				}
			}
			else
			{
				break;
			}

			ret = unzGoToNextFile(zipArchive);

			if (ret == UNZ_END_OF_LIST_OF_FILE)
			{
				break;
			}

			if (ret != UNZ_OK)
			{
				unzClose(zipArchive);
				return 0;
			}
		}

		unzClose(zipArchive);
	}
#if 0 // AWH - No 7zip
	else if ((idbuf[0] == '7') && (idbuf[1] == 'z') && (idbuf[2] == 0xbc) && (idbuf[3] == 0xaf)) 
	{	// it's 7zip
		CFileInStream archiveStream;
		SRes res;
		CSzArEx db;              /* 7z archive database structure */
		ISzAlloc allocImp;       /* memory functions for main pool */
		ISzAlloc allocTempImp;   /* memory functions for temporary pool */

		archiveStream.File = fopen(filename, "rb");
		if (!archiveStream.File)
		{
			return 0;
		}

		archiveStream.InStream.Read = SzFileReadImp;
		archiveStream.InStream.Seek = SzFileSeekImp;

		allocImp.Alloc = SzAlloc;
		allocImp.Free = SzFree;

		allocTempImp.Alloc = SzAllocTemp;
		allocTempImp.Free = SzFreeTemp;

		// init 7zip internals
		CrcGenerateTable();
		SzArEx_Init(&db);

		res = SzArEx_Open(&db, &archiveStream.InStream, &allocImp, &allocTempImp);
		if (res == SZ_OK)
		{
			int i;

			for (i = 0; i < db.db.NumFiles; i++)
			{
				CSzFileItem *item = db.db.Files + i;

				if (!item->IsDirectory)
				{
					if (filetoload != NULL)
					{
						if (!strcasecmp(filetoload, item->Name))
						{
							UInt32 blockIndex = 0xFFFFFFFF; /* it can have any value before first call (if outBuffer = 0) */
							Byte *outBuffer = 0; /* it must be 0 before first call for each new archive. */
							size_t outBufferSize = 0;  /* it can have any value before first call (if outBuffer = 0) */
							size_t offset;
							size_t outSizeProcessed;

							res = SzAr_Extract(&db, &archiveStream.InStream, i, 
							        &blockIndex, &outBuffer, &outBufferSize, 
							        &offset, &outSizeProcessed, 
							        &allocImp, &allocTempImp);
						
							if (res == SZ_OK)
							{
								SzArEx_Free(&db, &allocImp);
								fclose(archiveStream.File);

								*datasize = (int)outBufferSize;
								*dataout = (unsigned char *)outBuffer;
								*dataoffset = (int)offset;

								return 1;
							}
							else
							{
								std::cout << "Error extracting 7zip!\n";
							}
						}
					}

					if (checkExtension(item->Name))
					{
						char *tmpstr;

						tmpstr = (char *)malloc(strlen(item->Name)+1);
						strcpy(tmpstr, item->Name);

						// add to the file list
						filelist.push_back(tmpstr);
						filesFound++;
					}
				}
			}

			SzArEx_Free(&db, &allocImp);
			fclose(archiveStream.File);
		}
	}
#endif // AWH - 7zip
	else if ((idbuf[0] == 'R') && (idbuf[1] == 'a') && (idbuf[2] == 'r') && (idbuf[3] == '!')) 
	{	// it's rar 
		
	}

	// if we found any files and weren't forced to load them, handle accordingly
	if (filesFound)
	{
		// only 1 file found, just run it
		if (filesFound == 1)
		{
			char fname[512];
			
			strcpy(fname, filelist[0]);

			free(filelist[0]);
			filelist.clear();

			strcpy(outname, fname);

			return auxio_load_archive(filename, dataout, datasize, dataoffset, fname, NULL); 
		}
#if 0 // AWH
		else	// multiple files we can handle found, give the user a choice
		{
			int sel;
			char fname[512];

			GtkWidget *archselect = gtk_window_new(GTK_WINDOW_TOPLEVEL);
			gtk_window_set_title(GTK_WINDOW (archselect), "Pick game in archive");
			gtk_window_set_modal(GTK_WINDOW (archselect), TRUE);
			
			GtkWidget *archbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
			gtk_container_add(GTK_CONTAINER(archselect), archbox);
			gtk_widget_show(archbox);

			GtkWidget *scrolledwindow = gtk_scrolled_window_new(NULL, NULL);
			gtk_box_pack_start(GTK_BOX(archbox), scrolledwindow, TRUE, TRUE, 0);
			gtk_widget_set_size_request(scrolledwindow, 340, 340);
			gtk_widget_show(scrolledwindow);

			GtkWidget *buttonbox = gtk_widget_new(GTK_TYPE_BOX, "halign", GTK_ALIGN_END, NULL);
			gtk_box_pack_start(GTK_BOX(archbox), buttonbox, FALSE, TRUE, 0);
			gtk_widget_show(buttonbox);

			GtkWidget *archtree = gtk_tree_view_new();
			gtk_container_add(GTK_CONTAINER (scrolledwindow), archtree);
			gtk_tree_view_set_fixed_height_mode(GTK_TREE_VIEW (archtree), FALSE);
			g_signal_connect(G_OBJECT(archtree), "button_press_event", G_CALLBACK(check_list_double), NULL);
			gtk_widget_show(archtree);

			// set up our tree store
			treestore = gtk_tree_store_new(1, G_TYPE_STRING);

			// attach the store to the tree	
			gtk_tree_view_set_model(GTK_TREE_VIEW(archtree), GTK_TREE_MODEL(treestore));
			
			for (int fn = 0; fn < filelist.size(); fn++)
			{
				gtk_tree_store_insert(treestore, &treeiters[fn], NULL, 999999);
				gtk_tree_store_set(treestore, &treeiters[fn], 0, filelist[fn], -1);
			}

			// create a cell renderer using the stock text one
			renderer = gtk_cell_renderer_text_new();

			// create a display column using the renderer
			column = gtk_tree_view_column_new_with_attributes ("NES file",
		                                                   renderer,
		                                                   "text", 0,
		                                                   NULL);

			// add the display column and renderer to the tree view
			gtk_tree_view_append_column(GTK_TREE_VIEW (archtree), column);
			
			GtkWidget *archcancel = gtk_widget_new(GTK_TYPE_BUTTON, "label", GTK_STOCK_CANCEL, "halign", GTK_ALIGN_END, "margin-top", 8, "margin-bottom", 8, "margin-right", 8, NULL);
			gtk_button_set_use_stock(GTK_BUTTON(archcancel), TRUE);
			gtk_box_pack_start(GTK_BOX(buttonbox), archcancel, FALSE, FALSE, 0);
			gtk_widget_show(archcancel);

			GtkWidget *archok = gtk_widget_new(GTK_TYPE_BUTTON, "label", GTK_STOCK_OK, "halign", GTK_ALIGN_END, "margin-top", 8, "margin-bottom", 8, "margin-right", 8, NULL);
			gtk_button_set_use_stock(GTK_BUTTON(archok), TRUE);
			gtk_box_pack_start(GTK_BOX(buttonbox), archok, FALSE, FALSE, 0);
			gtk_widget_show(archok);

			// get the selection object too
			selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(archtree));

			g_signal_connect(G_OBJECT(archcancel), "clicked",
				G_CALLBACK(on_archcancel_clicked), NULL);

			g_signal_connect(G_OBJECT(archok), "clicked",
				G_CALLBACK(on_archok_clicked), NULL);
				
			g_signal_connect(G_OBJECT(archselect), "destroy",
				G_CALLBACK(on_archselect_destroyed), NULL);

			gtk_widget_show(archselect);
			
			run_picker = true;
			cancelled = false;

			while (run_picker)
			{
				gtk_main_iteration_do(TRUE);
			}

			sel = find_current_selection();

			gtk_widget_destroy(archselect);

			// was something picked?
			if ((sel != -1) && (!cancelled))
			{
				strcpy(fname, filelist[sel]);
			}

			// free all the temp filenames
			for (int fn = 0; fn < filelist.size(); fn++)
			{
				free(filelist[fn]);
			}

			// and wipe the vector
			filelist.clear();

			if ((sel != -1) && (!cancelled))
			{
				if (outname)
				{
					strcpy(outname, fname);
				}

				return auxio_load_archive(filename, dataout, datasize, dataoffset, fname, NULL); 
			}
		}
#endif // AWH
	}

	return 0;
}

void auxio_load_db(void)
{
	Nes::Api::Cartridge::Database database( emulator );
#if 0 /* AWH - Skip loading the override DB */
	char dirname[1024], datadirname[1024], *pwd;

	if (nstDBFile)
	{
		return;
	}

	pwd = getenv("PWD");
	sprintf(dirname, "%s/NstDatabase.xml", pwd);
	sprintf(datadirname, "%s/NstDatabase.xml", DATADIR);

	nstDBFile = new std::ifstream(datadirname, std::ifstream::in|std::ifstream::binary);

	if (nstDBFile->is_open())
	{
		database.Load(*nstDBFile);
		database.Enable(true);
	}
	else
	{
		printf("NstDatabase.xml not found in %s\n", DATADIR);
		nstDBFile = new std::ifstream(dirname, std::ifstream::in|std::ifstream::binary);

		if (nstDBFile->is_open())
		{
			database.Load(*nstDBFile);
			database.Enable(true);
		}
		else
		{
			printf("NstDatabase.xml not found in %s\nPAL detection and auto-ROM-fixing will be disabled\n", pwd);
			delete nstDBFile;
			nstDBFile = NULL;
		}
	}
#endif /* AWH */
}

