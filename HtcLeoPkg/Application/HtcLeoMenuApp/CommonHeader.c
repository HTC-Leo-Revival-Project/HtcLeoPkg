#include "CommonHeader.h"

void PutCharX( int x, int y, CHAR16 ch ) {
	CHAR16 chartab[ 2 ] = { 0 };
	chartab[ 0 ] = ch;

	SetCursorPos( x, y );
	Print( chartab );
}

BOOLEAN IsAlphanumeric( CHAR16 ch ) {
	return (ch >= 'a' && ch <= 'z') ||
		(ch >= 'A' && ch <= 'Z') ||
		(ch >= '0' && ch <= '9');
}

void SetCursorPos( int x, int y ) {
	gST->ConOut->SetCursorPosition( gST->ConOut, x, y );
}

void SetTextColor( UINT8 color ) {
	gST->ConOut->SetAttribute( gST->ConOut, EFI_TEXT_ATTR( color, EFI_BLACK ) );
}