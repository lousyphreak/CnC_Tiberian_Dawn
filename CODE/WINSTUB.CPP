/*
**	Command & Conquer(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : WINSTUB.CPP                                                  *
 *                                                                                             *
 *                   Programmer : Steve Tall                                                   *
 *                                                                                             *
 *                   Start Date : 10/04/95                                                     *
 *                                                                                             *
 *                  Last Update : October 4th 1995 [ST]                                        *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Overview:                                                                                   *
 *   This file contains stubs for undefined externals when linked under Watcom for Win 95      *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include	"FUNCTION.H"
#include "TCPIP.H"
#include "SDLINPUT.H"

extern int32_t BootstrapFocusSeen;
void Main_Window_Destroy(void);

void output(short,short)
{}

void Shake_Screen(int32_t)
{}

bool InDebugger = false;
bool ReadyToQuit = false;


uint32_t CCFocusMessage = WM_USER+50;	//Private message for receiving application focus
extern	void VQA_PauseAudio(void);
extern	void VQA_ResumeAudio(void);


ThemeType OldTheme = THEME_NONE;
extern BOOL InMovie;


/***********************************************************************************************
 * Focus_Loss -- this function is called when a library function detects focus loss            *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    2/1/96 2:10PM ST : Created                                                               *
 *=============================================================================================*/

void Focus_Loss(void)
{
	if (SoundOn){
		if (OldTheme == THEME_NONE){
			OldTheme = Theme.What_Is_Playing();
		}
	}
	Theme.Stop();
	Stop_Primary_Sound_Buffer();
	if (WWMouse) WWMouse->Clear_Cursor_Clip();
}


void Focus_Restore(void)
{
	Restore_Cached_Icons();
	Map.Flag_To_Redraw(true);
	Start_Primary_Sound_Buffer(TRUE);
	if (WWMouse) WWMouse->Set_Cursor_Clip();
	VisiblePage.Clear();
	HiddenPage.Clear();
}



/***********************************************************************************************
 * Check_For_Focus_Loss -- check for the end of the focus loss                                 *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    2/2/96 10:49AM ST : Created                                                              *
 *=============================================================================================*/

void Check_For_Focus_Loss(void)
{
	static BOOL	focus_last_time = 1;

	if ( !GameInFocus ){
		Focus_Loss();
		SDL_GameInput_Pump();
	}

	if (!focus_last_time && GameInFocus){

		VQA_PauseAudio();
		CountDownTimerClass cd;
		cd.Set(60*1);

		do {
			SDL_GameInput_Pump();
			if (AllDone) return;
			if (cd.Time()) {
				SDL_GameInput_Delay(1);
			}

		} while(cd.Time());
		VQA_ResumeAudio();
		Start_Primary_Sound_Buffer(TRUE);
		if (!InMovie){
			Theme.Queue_Song(OldTheme);
			OldTheme = THEME_NONE;
		}
		Focus_Restore();
	}

	focus_last_time = GameInFocus;

}

void Main_Window_Handle_Focus_Change(bool focused)
{
	if (focused) {
		BootstrapFocusSeen = TRUE;
		GameInFocus = TRUE;
		AllSurfaces.Set_Surface_Focus(TRUE);
		AllSurfaces.Restore_Surfaces();
	} else if (!BootstrapFocusSeen) {
		GameInFocus = FALSE;
		Focus_Loss();
		AllSurfaces.Set_Surface_Focus(FALSE);
		AllSurfaces.Restore_Surfaces();
	} else {
		GameInFocus = TRUE;
	}
}

void Main_Window_Handle_Close_Request(void)
{
	Main_Window_Destroy();
}

void Main_Window_Destroy(void)
{
	Prog_End();
	Invalidate_Cached_Icons();
	VisiblePage.Un_Init();
	HiddenPage.Un_Init();
	AllSurfaces.Release();
	if (!InDebugger) {
		Reset_Video_Mode();
	}
	if (MainWindow != NULL) {
		RA_DestroyWindow(MainWindow);
		MainWindow = NULL;
	}
	AllDone = TRUE;
	if (ReadyToQuit) {
		ReadyToQuit = 2;
	} else {
#ifndef DEMO
		Shutdown_Network();
#endif
		if (Winsock.Get_Connected()) Winsock.Close();
		exit(0);
	}
}










/***********************************************************************************************
 * Create_Main_Window -- opens the MainWindow for C&C                                          *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    instance -- handle to program instance                                            *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    10/10/95 4:08PM ST : Created                                                             *
 *=============================================================================================*/

#define CC_ICON 1
int ShowCommand;

void Create_Main_Window ( HANDLE instance ,int command_show , int width , int height )

{
	RAWindow *hwnd;

	(void)instance;
	hwnd = RA_CreateWindow("Command & Conquer", width, height, SDL_WINDOW_RESIZABLE);
	ShowWindow(hwnd, command_show);
	ShowCommand = command_show;
	MainWindow = hwnd;			//Save the handle to our main window
	hInstance = instance;

	Audio_Focus_Loss_Function = &Focus_Loss;
	Misc_Focus_Loss_Function = &Focus_Loss;
	Misc_Focus_Restore_Function = &Focus_Restore;
	Gbuffer_Focus_Loss_Function = &Focus_Loss;
}

void Window_Dialog_Box(HANDLE  hinst, LPCTSTR  lpszTemplate, HWND  hwndOwner, DLGPROC  dlgprc)
{
	(void)hinst;
	(void)lpszTemplate;
	(void)hwndOwner;
	(void)dlgprc;
	/*
	** Get rid of the Westwood mouse cursor because we are showing a
	** dialog box and we want to have the right windows cursor showing
	** for it.
	*/
	Hide_Mouse();
	Show_Mouse();
}



typedef struct tColourList {

	char	Red;
	char	Green;
	char	Blue;
} ColourList;

ColourList	ColourLookup[9]={
	0,0,0,
	63,0,0,
	0,63,0,
	0,0,63,
	63,0,63,
	63,63,0,
	0,63,63,
	32,32,32,
	63,63,63
};




int	DebugColour=1;


extern "C" void Set_Palette_Register(int number,int red ,int green ,int blue);
void Colour_Debug (int call_number)
{
}

BOOL Any_Locked (void)
{
	if (SeenBuff.Get_LockCount() ||
		HidPage.Get_LockCount()){
		return (TRUE);
	}else{
		return(FALSE);
	}
}







HANDLE DebugFile = INVALID_HANDLE_VALUE;

/***********************************************************************************************
 * CCDebugString -- sends a string to the debugger and echos it to disk                        *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    string                                                                            *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    10/28/96 12:48PM ST : Created                                                            *
 *=============================================================================================*/
void CCDebugString (char const *string)
{
	printf( "%s", string );
}














//
// Miscellaneous stubs. Mainly for multi player stuff
//
//
//

//UDPAddressClass::UDPAddressClass(void) {
//	int i;
//	i++;
//}
//int UDPManagerClass::Num_Connections(void){ return (0); }
//int UDPManagerClass::Connection_ID( int ) { return (0); }
//UDPAddressClass * UDPManagerClass::Connection_Address( int ) { return ((UDPAddressClass*)0); }
//char * UDPManagerClass::Connection_Name( int ) { return ((char*)0); }
//int UDPAddressClass::Is_Broadcast() { return (0); }
//int UDPManagerClass::Send_Global_Message( void *, int, int, UDPAddressClass * ) { return (0); }
//int UDPManagerClass::Service() { return (0); }
//int UDPManagerClass::Get_Global_Message( void  *, int  *, UDPAddressClass  *, short unsigned  * ) { return (0); }
//int UDPAddressClass::operator ==( UDPAddressClass  & ) { return (0); }
//UDPManagerClass::UDPManagerClass( int, int, int, int, short unsigned, short unsigned ) {}
//UDPManagerClass::~UDPManagerClass() {
//	int i;
//	i++;
//	}
//int  UDPManagerClass::Delete_Connection( int ) { return (0); }
//UDPAddressClass::UDPAddressClass( char unsigned  *, char unsigned  * ){}
//void  UDPManagerClass::Set_Socket( short unsigned ){}
//int  UDPManagerClass::Is_IPX() { return (0); }
//int  UDPManagerClass::Init() { return (0); }
//void  UDPAddressClass::Get_Address( char unsigned  *, char unsigned  * ){}
//void  UDPManagerClass::Set_Bridge( char unsigned  * ){}
//int  UDPManagerClass::Global_Num_Send() { return (0); }
//void  UDPManagerClass::Set_Timing( long unsigned, long unsigned, long unsigned ){}
//unsigned long UDPManagerClass::Global_Response_Time() { return (0); }
//int  UDPManagerClass::Create_Connection( int, char  *, UDPAddressClass  * ) { return (0); }
//int  UDPAddressClass::operator !=( UDPAddressClass  & ) { return (0); }
//int  UDPManagerClass::Send_Private_Message( void  *, int, int, int ) { return (0); }
//int  UDPManagerClass::Get_Private_Message( void  *, int  *, int  * ) { return (0); }
//int  UDPManagerClass::Connection_Index( int ) { return (0); }
//void  UDPManagerClass::Reset_Response_Time(){}
//long unsigned  UDPManagerClass::Response_Time() { return (0); }
//int  UDPManagerClass::Private_Num_Send( int ) { return (0); }

//_VQAHandle  *  VQA_Alloc(void){ return ((_VQAHandle *)0); }
//void  VQA_Init( _VQAHandle  *, long ( *)()) {}
//long  VQA_Open( _VQAHandle  *, char const  *, _VQAConfig  * ) { return (0); }
//void  VQA_Free( _VQAHandle  * ) {}
//void  VQA_Close( _VQAHandle  * ) {}
//long  VQA_Play( _VQAHandle  *, long ) { return (0); }

//void VQA_Init(VQAHandle *, long(*)(VQAHandle *vqa, long action,	void *buffer, long nbytes)){}

//long VQA_Open(VQAHandle *, char const *, VQAConfig *)
//{
//	return (0);
//}

//void VQA_Close(VQAHandle *){}

//long VQA_Play(VQAHandle *, long)
//{
//	return (0);
//}


/*
 * Check_VQ_Palette_Set: formerly used a deferred-palette mechanism (Flag_To_Set_Palette /
 * VQPaletteChange) to apply VQA palettes from the main loop. VQA playback now calls
 * SetPalette(uint8_t*, int32_t, uint32_t) directly via WIN32LIB/PALETTE/PALETTE.CPP, so
 * this hook is no longer needed. Kept as an empty stub because CONQUER.CPP's VQ_Call_Back
 * still calls it each frame.
 */
bool VQPaletteChange = false;

void Check_VQ_Palette_Set(void)
{
}


/***********************************************************************************************
 * Memory_Error_Handler -- Handle a possibly fatal failure to allocate memory                  *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    5/22/96 3:57PM ST : Created                                                              *
 *=============================================================================================*/
void Memory_Error_Handler(void)
{
	VisiblePage.Clear();
	Set_Palette(GamePalette);
	while (Get_Mouse_State()){Show_Mouse();};
	CCMessageBox().Process("Error - out of memory.", "Abort", false);
	ReadyToQuit = 1;
	Main_Window_Destroy();
}











GraphicBufferClass* Read_PCX_File(char* name, char* Palette, void *Buff, int32_t Size);



/***********************************************************************************************
 * Load_Title_Screen -- loads the title screen into the given video buffer                     *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    screen name                                                                       *
 *           video buffer                                                                      *
 *           ptr to buffer for palette                                                         *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    7/5/96 11:30AM ST : Created                                                              *
 *=============================================================================================*/

void Load_Title_Screen(char const *name, GraphicViewPortClass *video_page, unsigned char *palette)
{

	GraphicBufferClass *load_buffer;


	load_buffer = Read_PCX_File (const_cast<char *>(name), reinterpret_cast<char *>(palette), NULL, 0);

	if (load_buffer){
		load_buffer->Blit(*video_page);
		delete load_buffer;
	}
}



#include "FILEPCX.H"

/***************************************************************************
 * READ_PCX_FILE -- read a pcx file into a Graphic Buffer                  *
 *                                                                         *
 *	GraphicBufferClass* Read_PCX_File (char* name, char* palette ,void *Buff, long size );	*
 *  																								*
 *                                                                         *
 * INPUT: name is a NULL terminated string of the fromat [xxxx.pcx]        *
 *        palette is optional, if palette != NULL the the color palette of *
 *					 the pcx file will be place in the memory block pointed	   *
 *               by palette.																*
 *			 Buff is optinal, if Buff == NULL a new memory Buffer		 			*
 *					 will be allocated, otherwise the file will be placed 		*
 *					 at location pointd by Buffer;										*
 *			Size is the size in bytes of the memory block pointed by Buff		*
 *				  is also optional;
 *                                                                         *
 * OUTPUT: on succes a pointer to a GraphicBufferClass cointaining the     *
 *         pcx file, NULL othewise.                                        *
 *																									*
 * WARNINGS:                                                               *
 *         Appears to be a comment-free zone                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   05/03/1995 JRJ : Created.                                             *
 *   04/30/1996 ST : Tidied up and modified to use CCFileClass             *
 *=========================================================================*/

#define	POOL_SIZE 2048
#define	READ_CHAR()  *file_ptr++ ; \
							 if ( file_ptr	>= & pool [ POOL_SIZE ]	) { \
								 file_handle.Read (pool , POOL_SIZE ); \
								 file_ptr = pool ; \
							 }


GraphicBufferClass* Read_PCX_File(char* name, char* palette, void *Buff, int32_t Size)
{
	int						i, j;
	unsigned					rle;
	unsigned					color;
	int						scan_pos;
	char						*file_ptr;
	int						width;
	int						height;
	char						*buffer;
	PCX_HEADER				header;
	RGB						*pal;
	char						pool [POOL_SIZE];
	GraphicBufferClass	*pic;

	CCFileClass file_handle(name);

	if (!file_handle.Is_Available()) return (NULL);

	file_handle.Open(READ);

	file_handle.Read (&header, sizeof (PCX_HEADER));

	if (header.id != 10 &&  header.version != 5 && header.pixelsize != 8 ) return NULL ;

	width = header.width - header.x + 1;
	height = header.height - header.y + 1;

	if (Buff) {
    	buffer = (char *)Buff;
    	i = Size / width;
    	height = MIN (i - 1, height);
    	pic = new GraphicBufferClass(width, height, buffer, Size);
    	if ( !(pic && pic->Get_Buffer()) ) return NULL ;
	} else {
    	pic = new GraphicBufferClass(width, height, NULL, width*(height+4));
    	if ( !(pic && pic->Get_Buffer()) ) return NULL ;
	}

	buffer = (char *) pic->Get_Buffer();
	file_ptr = pool ;
	file_handle.Read (pool , POOL_SIZE);

	if ( header.byte_per_line != width ){

		for ( scan_pos = j = 0 ; j < height ; j ++, scan_pos += width ) {
			for ( i = 0 ; i < width ; ) {
				rle = READ_CHAR ();
				if ( rle > 192 ) {
					rle -= 192 ;
					color =	READ_CHAR (); ;
					memset ( buffer + scan_pos + i , color , rle );
					i += rle;
				} else {
					*(buffer+scan_pos + i++ ) = (char)rle;
				}
			}
      	}

		if ( i == width ) {
			rle = READ_CHAR ();
		}
		if ( rle > 192 ) {
			rle = READ_CHAR ();
		}

	} else {

		for ( i = 0 ; i < width * height ; ) {
  			rle = READ_CHAR ();
  			rle &= 0xff;
  			if ( rle > 192 ) {
        		rle -= 192 ;
        		color = READ_CHAR ();
  				memset ( buffer + i , color , rle );
        		i += rle ;
      		}else{
				*(buffer + i++) = (char)rle;
			}
		}
	}

	if ( palette ) {
		file_handle.Seek(-static_cast<int32_t>(256 * sizeof(RGB)), SEEK_END);
		file_handle.Read (palette , 256L * sizeof ( RGB ));

		pal = ( RGB * ) palette;
		for (i = 0 ; i < 256 ; i ++) {
			pal ->red	>>= 2;
			pal ->green	>>= 2;
			pal ->blue	>>= 2;
			pal ++ ;
		}
	}

	file_handle.Close();
	return pic;
}
