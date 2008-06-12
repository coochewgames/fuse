/* gtkjoystick.c: Joystick emulation
   Copyright (c) 2003-2008 Darren Salt, Philip Kendall, Marek Januszewski

   $Id$

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

   Author contact information:

   E-mail: philip-fuse@shadowmagic.org.uk

   Darren: linux@youmustbejoking.demon.co.uk

*/

#include <config.h>

#if !defined USE_JOYSTICK || defined HAVE_JSW_H
#include "../uijoystick.c"
#else
  /* FIXME: implement win32 joystick using mmsystem, not jsw */
#endif

#include <tchar.h>
#include <windows.h>

#include "fuse.h"
#include "joystick.h"
#include "keyboard.h"
#include "menu.h"
#include "settings.h"
#include "win32internals.h"

#include "win32joystick.h"

struct button_info {
  int *setting;
  HWND label;
  keyboard_key_name key;
};

struct joystick_info {

  int *type;
  HWND radio[ JOYSTICK_TYPE_COUNT ];

  struct button_info button[10];
};

static void setup_info( struct joystick_info *info, int action );
static INT_PTR CALLBACK dialog_proc( HWND hwndDlg, UINT uMsg,
                                     WPARAM wParam, LPARAM lParam );
static void dialog_init( HWND hwndDlg, struct joystick_info *info );
static void create_joystick_type_selector( struct joystick_info *info,
					   HWND hwndDlg );
static void
create_fire_button_selector( struct button_info *info, HWND hwndDlg );
static void set_key_text( HWND hlabel, keyboard_key_name key );
static void joystick_done( LONG user_data );
static void show_key_selection_popoup( HWND hwndDlg, LPARAM lParam );

void
menu_options_joysticks_select( int action )
{
  struct joystick_info info;

  fuse_emulation_pause();

  setup_info( &info, action );
  
  DialogBoxParam( fuse_hInstance, MAKEINTRESOURCE( IDD_JOYSTICKS ),
                  fuse_hWnd, dialog_proc, ( LPARAM ) &info );

  fuse_emulation_unpause();
}

static INT_PTR CALLBACK
dialog_proc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
  switch( uMsg ) {

    case WM_INITDIALOG:
      SetWindowLong( hwndDlg, GWL_USERDATA, lParam );
      dialog_init( hwndDlg, ( struct joystick_info * ) lParam );
      return FALSE;

    case WM_COMMAND:
      switch( LOWORD( wParam ) ) {
        case IDC_JOYSTICKS_BUTTON_BUTTON1:
        case IDC_JOYSTICKS_BUTTON_BUTTON2:
        case IDC_JOYSTICKS_BUTTON_BUTTON3:
        case IDC_JOYSTICKS_BUTTON_BUTTON4:
        case IDC_JOYSTICKS_BUTTON_BUTTON5:
        case IDC_JOYSTICKS_BUTTON_BUTTON6:
        case IDC_JOYSTICKS_BUTTON_BUTTON7:
        case IDC_JOYSTICKS_BUTTON_BUTTON8:
        case IDC_JOYSTICKS_BUTTON_BUTTON9:
        case IDC_JOYSTICKS_BUTTON_BUTTON10:
          show_key_selection_popoup( hwndDlg, lParam );
          return 0;
          
        case IDOK:
          joystick_done( GetWindowLong( hwndDlg, GWL_USERDATA ) );
          EndDialog( hwndDlg, 0 );
          return 0;

        case IDCANCEL:
          EndDialog( hwndDlg, 0 );
          return 0;
      }
      break;

    case WM_CLOSE:
      EndDialog( hwndDlg, 0 );
      return 0;      
  }
  return FALSE;
}

static void
dialog_init( HWND hwndDlg, struct joystick_info *info )
{
  size_t i;
  create_joystick_type_selector( info, hwndDlg );

  for( i = 0; i < 10; i++ ) {
    
    info->button[i].label = GetDlgItem( hwndDlg,
                                       IDC_JOYSTICKS_BUTTON_BUTTON1 + i );
    if( info->button[i].setting ) {
      create_fire_button_selector( &( info->button[i] ), hwndDlg );
    }
    else {
      /* disable the button configuration part of the dialog */
      SendMessage( info->button[i].label, WM_SETTEXT,
                   0, ( LPARAM ) TEXT( "N/A" ) );
      EnableWindow( info->button[i].label, FALSE );
      EnableWindow( GetDlgItem( hwndDlg,
                                IDC_JOYSTICKS_STATIC_BUTTON1 + i ), FALSE );
      EnableWindow( GetDlgItem( hwndDlg,
                                IDC_JOYSTICKS_GROUP_BUTTON1 + i ), FALSE );
    }
  }
}

static void
setup_info( struct joystick_info *info, int action )
{
  size_t i;

  switch( action ) {

  case 1:
    info->type = &( settings_current.joystick_1_output );
    info->button[0].setting = &( settings_current.joystick_1_fire_1  );
    info->button[1].setting = &( settings_current.joystick_1_fire_2  );
    info->button[2].setting = &( settings_current.joystick_1_fire_3  );
    info->button[3].setting = &( settings_current.joystick_1_fire_4  );
    info->button[4].setting = &( settings_current.joystick_1_fire_5  );
    info->button[5].setting = &( settings_current.joystick_1_fire_6  );
    info->button[6].setting = &( settings_current.joystick_1_fire_7  );
    info->button[7].setting = &( settings_current.joystick_1_fire_8  );
    info->button[8].setting = &( settings_current.joystick_1_fire_9  );
    info->button[9].setting = &( settings_current.joystick_1_fire_10 );
    break;

  case 2:
    info->type = &( settings_current.joystick_2_output );
    info->button[0].setting = &( settings_current.joystick_2_fire_1  );
    info->button[1].setting = &( settings_current.joystick_2_fire_2  );
    info->button[2].setting = &( settings_current.joystick_2_fire_3  );
    info->button[3].setting = &( settings_current.joystick_2_fire_4  );
    info->button[4].setting = &( settings_current.joystick_2_fire_5  );
    info->button[5].setting = &( settings_current.joystick_2_fire_6  );
    info->button[6].setting = &( settings_current.joystick_2_fire_7  );
    info->button[7].setting = &( settings_current.joystick_2_fire_8  );
    info->button[8].setting = &( settings_current.joystick_2_fire_9  );
    info->button[9].setting = &( settings_current.joystick_2_fire_10 );
    break;

  case 3:
    info->type = &( settings_current.joystick_keyboard_output );
    for( i = 0; i < 10; i++ ) info->button[i].setting = NULL;
    break;

  }
}

static void
create_joystick_type_selector( struct joystick_info *info, HWND hwndDlg )
{
  size_t i;
  HFONT font;
  RECT rect;

  font = ( HFONT ) SendMessage( hwndDlg, WM_GETFONT, 0, 0 );

  for( i = 0; i < JOYSTICK_TYPE_COUNT; i++ ) {

    rect.left = 5; rect.top = i * 10 + 10 ;
    rect.right = 5 + 45; rect.bottom = ( i * 10 ) + 10 + 10;
    MapDialogRect( hwndDlg, &rect );
    info->radio[ i ] = CreateWindowEx( 0, WC_BUTTON, joystick_name[ i ],
                                       WS_VISIBLE | WS_CHILD |
                                       WS_TABSTOP | BS_AUTORADIOBUTTON,
                                       rect.left, rect.top,
                                       rect.right - rect.left,
                                       rect.bottom - rect.top,
                                       hwndDlg, 0, fuse_hInstance, 0 );
    SendMessage( info->radio[ i ], WM_SETFONT, ( WPARAM ) font, FALSE );

    if( i == *( info->type ) ) 
      SendMessage( info->radio[ i ], BM_SETCHECK, BST_CHECKED, 0 );
  }
  
  rect.left = 0; rect.top = 0;
  rect.right = 60; rect.bottom = ( i * 10 ) + 10 + 10 + 5;
  MapDialogRect( hwndDlg, &rect );
  MoveWindow( GetDlgItem( hwndDlg, IDR_JOYSTICKS_POPUP ),
              rect.left, rect.top,
              rect.right - rect.left, rect.bottom - rect.top,
              FALSE );
}

static void
create_fire_button_selector( struct button_info *info, HWND hwndDlg )
{
  info->key = *info->setting;
  set_key_text( info->label, info->key );

  SetWindowLong( info->label, GWL_USERDATA, ( LONG ) info );
}

static void
set_key_text( HWND hlabel, keyboard_key_name key )
{
  const TCHAR *text;
  TCHAR buffer[40];

  text = keyboard_key_text( key );

  _sntprintf( buffer, 40, "%s", text );
  
  SendMessage( hlabel, WM_SETTEXT, 0, ( LPARAM ) buffer );
}

static void
joystick_done( LONG user_data )
{
  struct joystick_info *info = ( struct joystick_info * ) user_data;

  int i;

  for( i = 0; i < 10; i++ )
    if( info->button[i].setting )
      *info->button[i].setting = info->button[i].key;

  for( i = 0; i < JOYSTICK_TYPE_COUNT; i++ ) {

    if( SendMessage( info->radio[ i ], BM_GETCHECK, 0, 0 ) == BST_CHECKED ) {
      *( info->type ) = i;
      return;
    }
  }
}

static void
show_key_selection_popoup( HWND hwndDlg, LPARAM lParam )
{
  RECT rect;
  HMENU hpopup;
  struct button_info *info;
  BOOL menu_id;
  
  info = ( struct button_info * ) GetWindowLong( ( HWND ) lParam,
                                                 GWL_USERDATA );
  /* create a popup right over the button that has been clicked */
  GetWindowRect( ( HWND ) lParam, &rect );
  hpopup = GetSubMenu( LoadMenu( fuse_hInstance,
                     MAKEINTRESOURCE( IDR_JOYSTICKS_POPUP ) ), 0 );
  /* popup returns the key value */
  menu_id = TrackPopupMenu( hpopup, TPM_LEFTALIGN | TPM_TOPALIGN |
                            TPM_NONOTIFY | TPM_RETURNCMD |
                            TPM_RIGHTBUTTON, rect.left, rect.top, 0,
                            fuse_hWnd, NULL );
  if( menu_id > 0 ) {
    /* KEYBOARD_NONE is 0, and TrackPopupMenu returns 0 on error,
       so menu id for KEYBOARD_NONE is 1 to distiguish the 2 results */
    if( menu_id != 1 )
      info->key = menu_id;
    else
      info->key = KEYBOARD_NONE;
    set_key_text( info->label, info->key );
  }
}