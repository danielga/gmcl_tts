#include <GarrysMod/Lua/Interface.h>
#include <lua.hpp>

#include <cstdint>
#include <vector>
#include <string>

#include <sapi.h>

#pragma warning( push ) // Disable warning C4996: 'GetVersionExA': was declared deprecated (sphelper.h:1319)
#pragma warning( disable: 4996 )
#include <sphelper.h>
#pragma warning( pop )

namespace tts
{

static const char metaname[] = "ISpVoice";
static int32_t metatype = GarrysMod::Lua::Type::None;
static const char tablename[] = "tts";
static const char invalid_error[] = "invalid ISpVoice";

inline std::wstring ConvertToUTF16( const char *str )
{
	const int wchars_num = MultiByteToWideChar( CP_UTF8, 0, str, -1, nullptr, 0 );
	std::wstring wstr( wchars_num - 1, L'\0' );
	MultiByteToWideChar( CP_UTF8, 0, str, -1, wstr.data( ), wchars_num );
	return wstr;
}

inline std::string ConvertToUTF8( const wchar_t *wstr )
{
	BOOL used_default_char = false;
	const int chars_num = WideCharToMultiByte( CP_UTF8, 0, wstr, -1, nullptr, 0, '\0', &used_default_char );
	std::string str( chars_num - 1, '\0' );
	WideCharToMultiByte( CP_UTF8, 0, wstr, -1, str.data( ), chars_num, '\0', &used_default_char );
	return str;
}

static int32_t PushError( GarrysMod::Lua::ILuaBase *LUA, const char *error, HRESULT hr )
{
	LUA->PushNil( );
	LUA->PushString( error );
	LUA->PushNumber( hr );
	return 3;
}

inline int32_t PushEvent( GarrysMod::Lua::ILuaBase *LUA, const CSpEvent &ev )
{
	LUA->CreateTable( );

	LUA->PushNumber( ev.eEventId );
	LUA->SetField( -2, "event_id" );

	LUA->PushNumber( ev.elParamType );
	LUA->SetField( -2, "param_type" );

	LUA->PushNumber( ev.ulStreamNum );
	LUA->SetField( -2, "stream_num" );

	LUA->PushNumber( static_cast<double>( ev.ullAudioStreamOffset ) );
	LUA->SetField( -2, "stream_offset" );

	LUA->PushNumber( static_cast<double>( ev.wParam ) );
	LUA->SetField( -2, "wparam" );

	LUA->PushNumber( static_cast<double>( ev.lParam ) );
	LUA->SetField( -2, "lparam" );

	return 1;
}

inline void Check( GarrysMod::Lua::ILuaBase *LUA, int32_t index )
{
	if( !LUA->IsType( index, metatype ) )
		LUA->TypeError( index, metaname );
}

static ISpVoice *GetAndValidate( GarrysMod::Lua::ILuaBase *LUA, int32_t index )
{
	Check( LUA, index );
	ISpVoice *voice = LUA->GetUserType<ISpVoice>( index, metatype );
	if( voice == nullptr )
		LUA->ArgError( index, invalid_error );

	return voice;
}

inline int32_t SetVoice( GarrysMod::Lua::ILuaBase *LUA, ISpVoice *voice, const char *name )
{
	std::wstring wide = L"Name=" + ConvertToUTF16( name );

	CComPtr<ISpObjectToken> bestvoice;
	HRESULT hr = SpFindBestToken( SPCAT_VOICES, wide.c_str( ), nullptr, &bestvoice );
	if( FAILED( hr ) )
		return PushError( LUA, "find_failed", hr );

	hr = voice->SetVoice( bestvoice );
	if( FAILED( hr ) )
		return PushError( LUA, "change_failed", hr );

	return 0;
}

LUA_FUNCTION_STATIC( gc )
{
	ISpVoice *voice = LUA->GetUserType<ISpVoice>( 1, metatype );
	if( voice != nullptr )
	{
		voice->Release( );
		LUA->SetUserType( 1, nullptr );
	}

	return 0;
}

LUA_FUNCTION_STATIC( tostring )
{
	LUA->PushFormattedString( "%s: %p", metaname, GetAndValidate( LUA, 1 ) );
	return 1;
}

LUA_FUNCTION_STATIC( eq )
{
	LUA->PushBool( GetAndValidate( LUA, 1 ) == GetAndValidate( LUA, 2 ) );
	return 1;
}

LUA_FUNCTION_STATIC( index )
{
	LUA->GetMetaTable( 1 );
	LUA->Push( 2 );
	LUA->RawGet( -2 );
	if( !LUA->IsType( -1, GarrysMod::Lua::Type::NIL ) )
		return 1;

	LUA->Pop( 2 );

	LUA->GetFEnv( 1 );
	LUA->Push( 2 );
	LUA->RawGet( -2 );
	return 1;
}

LUA_FUNCTION_STATIC( newindex )
{
	LUA->GetFEnv( 1 );
	LUA->Push( 2 );
	LUA->Push( 3 );
	LUA->RawSet( -3 );
	return 0;
}

LUA_FUNCTION_STATIC( valid )
{
	Check( LUA, 1 );
	LUA->PushBool( LUA->GetUserType<ISpVoice>( 1, metatype ) != nullptr );
	return 1;
}

LUA_FUNCTION_STATIC( pause )
{
	ISpVoice *voice = GetAndValidate( LUA, 1 );

	HRESULT hr = voice->Pause( );
	if( FAILED( hr ) )
		return PushError( LUA, "pause_failed", hr );

	LUA->PushBool( true );
	return 1;
}

LUA_FUNCTION_STATIC( resume )
{
	ISpVoice *voice = GetAndValidate( LUA, 1 );

	HRESULT hr = voice->Resume( );
	if( FAILED( hr ) )
		return PushError( LUA, "resume_failed", hr );

	LUA->PushBool( true );
	return 1;
}

LUA_FUNCTION_STATIC( volume )
{
	ISpVoice *voice = GetAndValidate( LUA, 1 );

	HRESULT hr = S_OK;
	if( LUA->Top( ) > 1 )
	{
		hr = voice->SetVolume( static_cast<USHORT>( LUA->CheckNumber( 2 ) ) );
		if( FAILED( hr ) )
			return PushError( LUA, "volume_failed", hr );

		LUA->PushBool( true );
		return 1;
	}

	USHORT volume = 0;
	hr = voice->GetVolume( &volume );
	if( FAILED( hr ) )
		return PushError( LUA, "volume_failed", hr );

	LUA->PushNumber( volume );
	return 1;
}

LUA_FUNCTION_STATIC( rate )
{
	ISpVoice *voice = GetAndValidate( LUA, 1 );

	HRESULT hr = S_OK;
	if( LUA->Top( ) > 1 )
	{
		hr = voice->SetRate( static_cast<long>( LUA->CheckNumber( 2 ) ) );
		if( FAILED( hr ) )
			return PushError( LUA, "rate_failed", hr );

		LUA->PushBool( true );
		return 1;
	}

	long rate = 0;
	hr = voice->GetRate( &rate );
	if( FAILED( hr ) )
		return PushError( LUA, "rate_failed", hr );

	LUA->PushNumber( rate );
	return 1;
}

LUA_FUNCTION_STATIC( _state )
{
	ISpVoice *voice = GetAndValidate( LUA, 1 );

	SPVOICESTATUS status = { 0 };
	HRESULT hr = voice->GetStatus( &status, nullptr );
	if( FAILED( hr ) )
		return PushError( LUA, "wait_failed", hr );

	switch( status.dwRunningState )
	{
		case SPRS_DONE:
			LUA->PushString( "idle" );
			break;

		case SPRS_IS_SPEAKING:
			LUA->PushString( "speaking" );
			break;

		default:
			LUA->PushString( "unknown" );
			break;
	}

	return 1;
}

LUA_FUNCTION_STATIC( interest )
{
	ISpVoice *voice = GetAndValidate( LUA, 1 );
	int32_t top = LUA->Top( );
	ULONGLONG flags = 0;
	for( int32_t k = 2; k <= top; ++k )
		flags |= SPFEI( static_cast<ULONGLONG>( LUA->CheckNumber( k ) ) );

	HRESULT hr = voice->SetInterest( flags, flags );
	if( FAILED( hr ) )
		return PushError( LUA, "interest_failed", hr );

	LUA->PushBool( true );
	return 1;
}

LUA_FUNCTION_STATIC( skip )
{
	ISpVoice *voice = GetAndValidate( LUA, 1 );
	long num = static_cast<long>( LUA->CheckNumber( 2 ) );

	ULONG skipped = 0;
	HRESULT hr = voice->Skip( L"SENTENCE", num, &skipped );
	if( FAILED( hr ) )
		return PushError( LUA, "interest_failed", hr );

	LUA->PushNumber( skipped );
	return 1;
}

LUA_FUNCTION_STATIC( events )
{
	ISpVoice *voice = GetAndValidate( LUA, 1 );
	ULONG num = 1;

	if( LUA->Top( ) > 1 )
		num = static_cast<ULONG>( LUA->CheckNumber( 2 ) );

	std::vector<CSpEvent> events( num );
	HRESULT hr = voice->GetEvents( num, events.data( ), &num );
	if( FAILED( hr ) )
		return PushError( LUA, "retrieval_failed", hr );

	LUA->CreateTable( );

	for( ULONG k = 0; k < num; ++k )
	{
		LUA->PushNumber( k + 1 );
		PushEvent( LUA, events[k] );
		LUA->SetTable( -3 );
	}

	return 1;
}

LUA_FUNCTION_STATIC( voice )
{
	ISpVoice *voice = GetAndValidate( LUA, 1 );
	if( LUA->Top( ) > 1 )
	{
		int32_t res = SetVoice( LUA, voice, LUA->CheckString( 2 ) );
		if( res != 0 )
			return res;

		LUA->PushBool( true );
		return 1;
	}

	CComPtr<ISpObjectToken> token;
	HRESULT hr = voice->GetVoice( &token );
	if( FAILED( hr ) )
		return PushError( LUA, "retrieval_failed", hr );

	LPWSTR name = nullptr;
	hr = token->GetStringValue( nullptr, &name );
	if( FAILED( hr ) )
		return PushError( LUA, "name_failed", hr );

	std::string narrow = ConvertToUTF8( name );

	LUA->PushString( narrow.c_str( ) );
	return 1;
}

LUA_FUNCTION_STATIC( speak )
{
	ISpVoice *voice = GetAndValidate( LUA, 1 );
	const char *text = LUA->CheckString( 2 );
	bool xml = false;

	if( LUA->Top( ) > 2 )
	{
		LUA->CheckType( 3, GarrysMod::Lua::Type::BOOL );
		xml = LUA->GetBool( 3 );
	}

	std::wstring wide = ConvertToUTF16( text );

	ULONG stream_num = 0;
	HRESULT hr = voice->Speak(
		wide.c_str( ),
		SPF_ASYNC | ( xml ? SPF_IS_XML : SPF_DEFAULT ),
		&stream_num
	);
	if( FAILED( hr ) )
		return PushError( LUA, "speak_failed", hr );

	LUA->PushNumber( stream_num );
	return 1;
}

LUA_FUNCTION_STATIC( create )
{
	const char *voicename = nullptr;
	if( LUA->Top( ) > 0 )
		voicename = LUA->CheckString( 1 );

	CComPtr<ISpVoice> voice = nullptr;
	HRESULT hr = CoCreateInstance(
		CLSID_SpVoice,
		NULL,
		CLSCTX_ALL,
		IID_ISpVoice,
		reinterpret_cast<void **>( &voice )
	);
	if( FAILED( hr ) )
		return PushError( LUA, "creation_failed", hr );

	if( voicename != nullptr )
	{
		int32_t res = SetVoice( LUA, voice, voicename );
		if( res != 0 )
			return res;
	}

	LUA->PushUserType( voice.Detach( ), metatype );

	LUA->CreateTable( );
	LUA->SetFEnv( -2 );

	return 1;
}

LUA_FUNCTION_STATIC( voices )
{
	CComPtr<IEnumSpObjectTokens> voices;
	HRESULT hr = SpEnumTokens( SPCAT_VOICES, nullptr, nullptr, &voices );
	if( FAILED( hr ) )
		return PushError( LUA, "retrieval_failed", hr );

	ULONG num = 0;
	hr = voices->GetCount( &num );
	if( FAILED( hr ) )
		return PushError( LUA, "count_failed", hr );

	LUA->CreateTable( );

	ULONG idx = 0;
	while( num-- != 0 )
	{
		CComPtr<ISpObjectToken> token;
		hr = voices->Next( 1, &token, nullptr );
		if( FAILED( hr ) )
			return PushError( LUA, "enum_failed", hr );

		LPWSTR value = nullptr;
		hr = token->GetStringValue( nullptr, &value );
		if( FAILED( hr ) )
			return PushError( LUA, "identifier_failed", hr );

		std::string narrow = ConvertToUTF8( value );

		LUA->PushNumber( ++idx );
		LUA->PushString( narrow.c_str( ) );
		LUA->SetTable( -3 );
	}

	return 1;
}

static void Initialize( GarrysMod::Lua::ILuaBase *LUA )
{
	if( FAILED( CoInitialize( nullptr ) ) )
		LUA->ThrowError( "failed to initialize COM interfaces" );

	metatype = LUA->CreateMetaTable( metaname );

	LUA->PushCFunction( gc );
	LUA->SetField( -2, "__gc" );

	LUA->PushCFunction( tostring );
	LUA->SetField( -2, "__tostring" );

	LUA->PushCFunction( eq );
	LUA->SetField( -2, "__eq" );

	LUA->PushCFunction( index );
	LUA->SetField( -2, "__index" );

	LUA->PushCFunction( newindex );
	LUA->SetField( -2, "__newindex" );

	LUA->PushCFunction( valid );
	LUA->SetField( -2, "valid" );

	LUA->PushCFunction( pause );
	LUA->SetField( -2, "pause" );

	LUA->PushCFunction( resume );
	LUA->SetField( -2, "resume" );

	LUA->PushCFunction( volume );
	LUA->SetField( -2, "volume" );

	LUA->PushCFunction( rate );
	LUA->SetField( -2, "rate" );

	LUA->PushCFunction( _state );
	LUA->SetField( -2, "state" );

	LUA->PushCFunction( interest );
	LUA->SetField( -2, "interest" );

	LUA->PushCFunction( skip );
	LUA->SetField( -2, "skip" );

	LUA->PushCFunction( events );
	LUA->SetField( -2, "events" );

	LUA->PushCFunction( voice );
	LUA->SetField( -2, "voice" );

	LUA->PushCFunction( speak );
	LUA->SetField( -2, "speak" );

	LUA->Pop( 1 );

	LUA->PushSpecial( GarrysMod::Lua::SPECIAL_GLOB );

	LUA->CreateTable( );

	LUA->PushCFunction( create );
	LUA->SetField( -2, "create" );

	LUA->PushCFunction( voices );
	LUA->SetField( -2, "voices" );

	LUA->SetField( -2, tablename );

	LUA->Pop( 1 );
}

static void Deinitialize( GarrysMod::Lua::ILuaBase *LUA )
{
	LUA->PushSpecial( GarrysMod::Lua::SPECIAL_GLOB );

	LUA->PushNil( );
	LUA->SetField( -2, tablename );

	LUA->Pop( 1 );

	LUA->PushSpecial( GarrysMod::Lua::SPECIAL_REG );

	LUA->PushNil( );
	LUA->SetField( -2, metaname );

	LUA->Pop( 1 );

	::CoUninitialize( );
}

}

GMOD_MODULE_OPEN( )
{
	tts::Initialize( LUA );
	return 0;
}

GMOD_MODULE_CLOSE( )
{
	tts::Deinitialize( LUA );
	return 0;
}
