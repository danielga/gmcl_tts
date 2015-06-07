#include <GarrysMod/Lua/Interface.h>
#include <lua.hpp>
#include <cstdint>
#include <sapi.h>
#include <sphelper.h>
#include <vector>
#include <locale>
#include <codecvt>
#include <string>

namespace tts
{

static const char *metaname = "ISpVoice";
static uint8_t metatype = 170;
static const char *tablename = "tts";
static const char *invalid_error = "invalid ISpVoice";

struct userdata
{
	ISpVoice *voice;
	uint8_t type;
};

inline std::wstring ConvertToUTF16( const char *str )
{
	std::wstring_convert< std::codecvt_utf8_utf16<wchar_t> > converter;
	return converter.from_bytes( str );
}

inline std::string ConvertToUTF8( const wchar_t *str )
{
	std::wstring_convert< std::codecvt_utf8_utf16<wchar_t> > converter;
	return converter.to_bytes( str );
}

static int32_t PushError( lua_State *state, const char *error, HRESULT hr )
{
	LUA->PushNil( );
	LUA->PushString( error );
	LUA->PushNumber( hr );
	return 3;
}

inline int32_t PushEvent( lua_State *state, const CSpEvent &ev )
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

	LUA->PushNumber( ev.wParam );
	LUA->SetField( -2, "wparam" );

	LUA->PushNumber( ev.lParam );
	LUA->SetField( -2, "lparam" );

	return 1;
}

inline void Check( lua_State *state, int32_t index )
{
	if( !LUA->IsType( index, metatype ) )
		luaL_typerror( state, index, metaname );
}

inline userdata *GetUserdata( lua_State *state, int32_t index )
{
	return static_cast<userdata *>( LUA->GetUserdata( index ) );
}

static ISpVoice *GetAndValidate( lua_State *state, int32_t index )
{
	Check( state, index );
	ISpVoice *voice = GetUserdata( state, index )->voice;
	if( voice == nullptr )
		LUA->ArgError( index, invalid_error );

	return voice;
}

static userdata *Create( lua_State *state )
{
	userdata *udata = static_cast<userdata *>( LUA->NewUserdata( sizeof( userdata ) ) );
	udata->type = metatype;

	LUA->CreateMetaTableType( metaname, metatype );
	LUA->SetMetaTable( -2 );

	LUA->CreateTable( );
	lua_setfenv( state, -2 );

	return udata;
}

inline int32_t SetVoice( lua_State *state, ISpVoice *voice, const char *name )
{
	std::wstring wide = L"Name=" + ConvertToUTF16( name );

	CComPtr<ISpObjectToken> bestvoice;
	HRESULT hr = SpFindBestToken( SPCAT_VOICES, wide.c_str( ), nullptr, &bestvoice );
	if( FAILED( hr ) )
		return PushError( state, "find_failed", hr );

	hr = voice->SetVoice( bestvoice );
	if( FAILED( hr ) )
		return PushError( state, "change_failed", hr );

	return 0;
}

LUA_FUNCTION_STATIC( gc )
{
	Check( state, 1 );
	userdata *udata = GetUserdata( state, 1 );

	ISpVoice *voice = udata->voice;
	if( voice != nullptr )
	{
		voice->Release( );
		udata->voice = nullptr;
	}

	return 0;
}

LUA_FUNCTION_STATIC( tostring )
{
	lua_pushfstring( state, "%s: %p", metaname, GetAndValidate( state, 1 ) );
	return 1;
}

LUA_FUNCTION_STATIC( eq )
{
	LUA->PushBool( GetAndValidate( state, 1 ) == GetAndValidate( state, 2 ) );
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

	lua_getfenv( state, 1 );
	LUA->Push( 2 );
	LUA->RawGet( -2 );
	return 1;
}

LUA_FUNCTION_STATIC( newindex )
{
	lua_getfenv( state, 1 );
	LUA->Push( 2 );
	LUA->Push( 3 );
	LUA->RawSet( -3 );
	return 0;
}

LUA_FUNCTION_STATIC( valid )
{
	Check( state, 1 );
	LUA->PushBool( GetUserdata( state, 1 )->voice != nullptr );
	return 1;
}

LUA_FUNCTION_STATIC( pause )
{
	ISpVoice *voice = GetAndValidate( state, 1 );

	HRESULT hr = voice->Pause( );
	if( FAILED( hr ) )
		return PushError( state, "pause_failed", hr );

	LUA->PushBool( true );
	return 1;
}

LUA_FUNCTION_STATIC( resume )
{
	ISpVoice *voice = GetAndValidate( state, 1 );

	HRESULT hr = voice->Resume( );
	if( FAILED( hr ) )
		return PushError( state, "resume_failed", hr );

	LUA->PushBool( true );
	return 1;
}

LUA_FUNCTION_STATIC( volume )
{
	ISpVoice *voice = GetAndValidate( state, 1 );

	HRESULT hr = S_OK;
	if( LUA->Top( ) > 1 )
	{
		hr = voice->SetVolume( static_cast<USHORT>( LUA->CheckNumber( 2 ) ) );
		if( FAILED( hr ) )
			return PushError( state, "volume_failed", hr );

		LUA->PushBool( true );
		return 1;
	}

	USHORT volume = 0;
	hr = voice->GetVolume( &volume );
	if( FAILED( hr ) )
		return PushError( state, "volume_failed", hr );

	LUA->PushNumber( volume );
	return 1;
}

LUA_FUNCTION_STATIC( rate )
{
	ISpVoice *voice = GetAndValidate( state, 1 );

	HRESULT hr = S_OK;
	if( LUA->Top( ) > 1 )
	{
		hr = voice->SetRate( static_cast<long>( LUA->CheckNumber( 2 ) ) );
		if( FAILED( hr ) )
			return PushError( state, "rate_failed", hr );

		LUA->PushBool( true );
		return 1;
	}

	long rate = 0;
	hr = voice->GetRate( &rate );
	if( FAILED( hr ) )
		return PushError( state, "rate_failed", hr );

	LUA->PushNumber( rate );
	return 1;
}

LUA_FUNCTION_STATIC( _state )
{
	ISpVoice *voice = GetAndValidate( state, 1 );

	SPVOICESTATUS status = { 0 };
	HRESULT hr = voice->GetStatus( &status, nullptr );
	if( FAILED( hr ) )
		return PushError( state, "wait_failed", hr );

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
	ISpVoice *voice = GetAndValidate( state, 1 );
	int32_t top = LUA->Top( );
	ULONGLONG flags = 0;
	for( int32_t k = 2; k <= top; ++k )
		flags |= SPFEI( static_cast<ULONGLONG>( LUA->CheckNumber( k ) ) );

	HRESULT hr = voice->SetInterest( flags, flags );
	if( FAILED( hr ) )
		return PushError( state, "interest_failed", hr );

	LUA->PushBool( true );
	return 1;
}

LUA_FUNCTION_STATIC( skip )
{
	ISpVoice *voice = GetAndValidate( state, 1 );
	long num = static_cast<long>( LUA->CheckNumber( 2 ) );

	ULONG skipped = 0;
	HRESULT hr = voice->Skip( L"SENTENCE", num, &skipped );
	if( FAILED( hr ) )
		return PushError( state, "interest_failed", hr );

	LUA->PushNumber( skipped );
	return 1;
}

LUA_FUNCTION_STATIC( events )
{
	ISpVoice *voice = GetAndValidate( state, 1 );
	ULONG num = 1;

	if( LUA->Top( ) > 1 )
		num = static_cast<ULONG>( LUA->CheckNumber( 2 ) );

	std::vector<CSpEvent> events( num );
	HRESULT hr = voice->GetEvents( num, events.data( ), &num );
	if( FAILED( hr ) )
		return PushError( state, "retrieval_failed", hr );

	LUA->CreateTable( );

	for( ULONG k = 0; k < num; ++k )
	{
		LUA->PushNumber( k + 1 );
		PushEvent( state, events[k] );
		LUA->SetTable( -3 );
	}

	return 1;
}

LUA_FUNCTION_STATIC( voice )
{
	ISpVoice *voice = GetAndValidate( state, 1 );
	if( LUA->Top( ) > 1 )
	{
		int32_t res = SetVoice( state, voice, LUA->CheckString( 2 ) );
		if( res != 0 )
			return res;

		LUA->PushBool( true );
		return 1;
	}

	CComPtr<ISpObjectToken> token;
	HRESULT hr = voice->GetVoice( &token );
	if( FAILED( hr ) )
		return PushError( state, "retrieval_failed", hr );

	LPWSTR name = nullptr;
	hr = token->GetStringValue( nullptr, &name );
	if( FAILED( hr ) )
		return PushError( state, "name_failed", hr );

	std::string narrow = ConvertToUTF8( name );

	LUA->PushString( narrow.c_str( ) );
	return 1;
}

LUA_FUNCTION_STATIC( speak )
{
	ISpVoice *voice = GetAndValidate( state, 1 );
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
		return PushError( state, "speak_failed", hr );

	LUA->PushNumber( stream_num );
	return 1;
}

LUA_FUNCTION_STATIC( create )
{
	userdata *udata = Create( state );
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
		return PushError( state, "creation_failed", hr );

	if( voicename != nullptr )
	{
		int32_t res = SetVoice( state, voice, voicename );
		if( res != 0 )
			return res;
	}

	udata->voice = voice.Detach( );
	return 1;
}

LUA_FUNCTION_STATIC( voices )
{
	CComPtr<IEnumSpObjectTokens> voices;
	HRESULT hr = SpEnumTokens( SPCAT_VOICES, nullptr, nullptr, &voices );
	if( FAILED( hr ) )
		return PushError( state, "retrieval_failed", hr );

	ULONG num = 0;
	hr = voices->GetCount( &num );
	if( FAILED( hr ) )
		return PushError( state, "count_failed", hr );

	LUA->CreateTable( );

	ULONG idx = 0;
	while( num-- != 0 )
	{
		CComPtr<ISpObjectToken> token;
		hr = voices->Next( 1, &token, nullptr );
		if( FAILED( hr ) )
			return PushError( state, "enum_failed", hr );

		LPWSTR value = nullptr;
		hr = token->GetStringValue( nullptr, &value );
		if( FAILED( hr ) )
			return PushError( state, "identifier_failed", hr );

		std::string narrow = ConvertToUTF8( value );

		LUA->PushNumber( ++idx );
		LUA->PushString( narrow.c_str( ) );
		LUA->SetTable( -3 );
	}

	return 1;
}

static void Initialize( lua_State *state )
{
	if( FAILED( CoInitialize( nullptr ) ) )
		LUA->ThrowError( "failed to initialize COM interfaces" );

	LUA->CreateMetaTableType( metaname, metatype );

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

static void Deinitialize( lua_State *state )
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
	tts::Initialize( state );
	return 0;
}

GMOD_MODULE_CLOSE( )
{
	tts::Deinitialize( state );
	return 0;
}