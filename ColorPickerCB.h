/*|*\
|*|  File:      ColorPickerCB.h
|*|  By:        James R. Twine
|*|             Copyright 1998, James R. Twine
|*|             Copyright 1999-2000, TransactionWorks, Inc.
|*|  Ported to pure Win32 (no MFC).
|*|
|*|             This implements a ComboBox control that can be
|*|             used to display and provide selection for a specific
|*|             set of colors. The standard set of colors provided
|*|             by the control are a subset of the X11 colorset, and
|*|             are the ones available in (and the color names
|*|             recognized by) Internet Explorer.
|*|
|*|             The ComboBox must have the Owner Draw Fixed and
|*|             Has Strings styles.
|*|
|*|             This is based on code that was originally found on
|*|             CodeGuru, and was (c) 1997 Baldvin Hansson.
|*|
|*|             This Code May Be Freely Incorporated Into
|*|             Projects Of Any Type Subject To The Following
|*|             Conditions:
|*|
|*|             o This Header Must Remain In This File, And Any
|*|               Files Derived From It
|*|             o Do Not Misrepresent The Origin Of Any Parts Of
|*|               This Code (IOW, Do Not Claim You Wrote It)
|*|
\*|*/
#if !defined(COLORPICKERCB_H__C74333B7_A13A_11D1_ADB6_C04D0BC10000__INCLUDED_)
#define COLORPICKERCB_H__C74333B7_A13A_11D1_ADB6_C04D0BC10000__INCLUDED_

#include "stdafx.h"

//
//	Constants...
//
#define		CCB_MAX_COLOR_NAME		32						// Max Chars For Color Name - 1

//
//	Macros...
//
#if	!defined( COUNTOF )
#define		COUNTOF( Array )	( ( sizeof( Array ) / sizeof( Array[ 0 ] ) ) )
#endif

//
//	Internal Structure For Color/Name Storage...
//
struct	SColorAndName
{
	SColorAndName()													// Default Constructor
	{
		memset( this, 0, sizeof( SColorAndName ) );					// Init Structure
	};
	SColorAndName( COLORREF crColor,
		const wchar_t* cpColor ) : m_crColor( crColor )			// Smart Constructor
	{
		wcsncpy_s( m_cColor, CCB_MAX_COLOR_NAME, cpColor, CCB_MAX_COLOR_NAME - 1 );	// Set Color Name
		m_cColor[ CCB_MAX_COLOR_NAME - 1 ] = L'\0';					// Just To Make Sure...
	};
	COLORREF	m_crColor;											// Actual Color RGB Value
	wchar_t		m_cColor[ CCB_MAX_COLOR_NAME ];						// Actual Name For Color
};

// A Win32 helper that wraps a plain HCBT combobox HWND and implements
// the owner-draw color picker functionality of the old MFC CColorPickerCB.
class CColorPickerCB
{
public:
	CColorPickerCB();
	virtual	~CColorPickerCB();

	// attach to an existing combobox control inside a dialog
	void Attach(HWND hCombo);
	HWND GetHwnd() const { return m_hCombo; }

	void			InitializeDefaultColors( void );			// Initialize Control With Default Colors

	COLORREF		GetSelectedColorValue( void );				// Get Selected Color Value
	std::wstring	GetSelectedColorName( void );				// Get Selected Color Name

	void			SetSelectedColorValue( COLORREF crClr );	// Set Selected Color Value
	void			SetSelectedColorName( const std::wstring& sName );	// Set Selected Color Name

	bool			RemoveColor( const std::wstring& sColor );	// Remove Color From List
	bool			RemoveColor( COLORREF crClr );				// Remove Color From List

	int				AddColor( const std::wstring& sName,
		COLORREF crColor );									// Add A New Color

	// owner draw helpers; call them from the dialog proc on
	// WM_MEASUREITEM / WM_DRAWITEM for the picker's combo box ids
	static void MeasureItem(HWND hCombo, LPMEASUREITEMSTRUCT lpMIS);
	static void DrawItem(HWND hCombo, LPDRAWITEMSTRUCT lpDIS);

private:
	HWND m_hCombo;

	//
	//	Prevent Misuse Of Copies...
	//
	CColorPickerCB( const CColorPickerCB& rSrc );
	CColorPickerCB	&operator=( const CColorPickerCB& rSrc );
};

/////////////////////////////////////////////////////////////////////////////

#endif // !defined(COLORPICKERCB_H__C74333B7_A13A_11D1_ADB6_C04D0BC10000__INCLUDED_)
