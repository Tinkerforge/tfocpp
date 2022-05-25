/*
 Simple, STB-style, parser for URL:s as specified by RFC1738 ( http://www.ietf.org/rfc/rfc1738.txt )

 compile with URL_PARSER_IMPLEMENTATION defined for implementation.
 compile with URL_PARSER_IMPLEMENTATION_STATIC defined for static implementation.

 version 1.0, June, 2014

 Copyright (C) 2014- Fredrik Kihlander

 This software is provided 'as-is', without any express or implied
 warranty.  In no event will the authors be held liable for any damages
 arising from the use of this software.

 Permission is granted to anyone to use this software for any purpose,
 including commercial applications, and to alter it and redistribute it
 freely, subject to the following restrictions:

 1. The origin of this software must not be misrepresented; you must not
 claim that you wrote the original software. If you use this software
 in a product, an acknowledgment in the product documentation would be
 appreciated but is not required.
 2. Altered source versions must be plainly marked as such, and must not be
 misrepresented as being the original software.
 3. This notice may not be removed or altered from any source distribution.

 Fredrik Kihlander
 */

#ifndef URL_H_INCLUDED
#define URL_H_INCLUDED

#include <stdlib.h>

#if defined(URL_PARSER_IMPLEMENTATION_STATIC)
#  if !defined(URL_PARSER_IMPLEMENTATION)
#    define URL_PARSER_IMPLEMENTATION
#    define URL_PARSER_LINKAGE static
#  endif
#else
#    define URL_PARSER_LINKAGE
#endif

/**
 * Struct describing a parsed url.
 *
 * @example <scheme>://<user>:<pass>@<host>:<port>/<path>?<query>#<fragment>
 */
struct parsed_url
{
	/**
	 * scheme part of url or 0x0 if not present.
	 * @note the scheme will be lower-cased!
	 */
	const char*  scheme;

	/**
	 * user part of url or 0x0 if not present.
	 */
	const char*  user;

	/**
	 * password part of url or 0x0 if not present.
	 */
	const char*  pass;

	/**
	 * host part of url or "localhost" if not present.
	 * if the host is an ipv6 address, i.e. enclosed by [] such as
	 * http://[::1]/whoppa host will be the string in question.
	 * It will also be verified that it is a valid ipv6 address, parsing
	 * will have failed if anything that is not an ipv6 address was found
	 * within a []
	 * @note the scheme will be lower-cased!
	 */
	const char*  host;

	/**
	 * port part of url.
	 * if not present a default depending on scheme is used, if no default is
	 * available for scheme, 0 will be used.
	 *
	 * supported defaults:
	 * "http"   - 80
	 * "https"  - 443
	 * "ftp"    - 21
	 * "ssh"    - 22
	 * "telnet" - 23
	 */
	unsigned int port;

	/**
	 * path part of url.
	 * if the path part of the url is not present, it will default to "/"
	 * @note percent-encoded values will get decoded during parse, i.e. %21 will be translated
	 *       to '!' etc.
	 *       see: https://en.wikipedia.org/wiki/Percent-encoding
	 */
	const char*  path;

	/**
	 * query part of url, default to 0x0 if not present in url.
	 * as this is not standardized it is not parsed for the user.
	 */
	 const char*  query;

	/**
	 * fragment part of url, default to 0x0 if not present in url.
	 */
	 const char*  fragment;
};

/**
 * Calculate the amount of memory needed to parse the specified url.
 * @param url the url to parse.
 */
URL_PARSER_LINKAGE size_t parse_url_calc_mem_usage(const char* url);

/**
 * Parse an url specified by RFC1738 into its parts.
 *
 * @param url url to parse.
 * @param mem memory-buffer to use to parse the url or NULL to use malloc.
 * @param mem_size size of mem in bytes.
 *
 * @return parsed url. If mem is NULL this value will need to be free:ed with free().
 */
URL_PARSER_LINKAGE parsed_url* parse_url(const char* url, void* mem, size_t mem_size);




#if defined(URL_PARSER_IMPLEMENTATION)
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

struct parse_url_ctx
{
	void* mem;
	size_t memsize;
	size_t memleft;
};

const char* parse_url_strnchr( const char* str, size_t len, int ch )
{
	for( size_t i = 0; i < len; ++i )
		if( str[i] == ch )
			return &str[i];
	return 0x0;
}

static void* parse_url_alloc_mem( parse_url_ctx* ctx, size_t request_size )
{
	if( request_size > ctx->memleft )
		return 0;
	void* res = (char*)ctx->mem + ctx->memsize - ctx->memleft;
	ctx->memleft -= request_size;
	return res;
}

static unsigned int parse_url_default_port_for_scheme( const char* scheme )
{
	if( scheme == 0x0 )
		return 0;

	if( strcmp( scheme, "http"   ) == 0 ) return 80;
	if( strcmp( scheme, "https"  ) == 0 ) return 443;
	if( strcmp( scheme, "ftp"    ) == 0 ) return 21;
	if( strcmp( scheme, "ssh"    ) == 0 ) return 22;
	if( strcmp( scheme, "telnet" ) == 0 ) return 23;
	return 0x0;
}

static char* parse_url_alloc_string( parse_url_ctx* ctx, const char* src, size_t len)
{
	char* dst = (char*)parse_url_alloc_mem( ctx, len + 1 );
	if( dst == 0x0 )
		return 0x0;
	memcpy( dst, src, len );
	dst[len] = '\0';
	return dst;
}

static const char* parse_url_alloc_lower_string( parse_url_ctx* ctx, const char* src, size_t len)
{
	char* dst = (char*)parse_url_alloc_mem( ctx, len + 1 );
	if( dst == 0x0 )
		return 0x0;
	// parse_url_strncpy_lower( new_str, src, len );
	for( size_t i = 0; i < len; ++i )
		dst[i] = (char)tolower( src[i] );
	dst[len] = '\0';
	return dst;
}

static const char* parse_url_parse_scheme( const char* url, parse_url_ctx* ctx, parsed_url* out )
{
	const char* schemesep = strchr( url, ':' );
	if( schemesep == 0x0 )
		return url;
	else
	{
		// ... is this the user part of a user/pass pair or the separator host:port? ...
		if( schemesep[1] != '/')
			return url;

		if( schemesep[2] != '/' )
			return 0x0;

		out->scheme = parse_url_alloc_lower_string( ctx, url, (size_t)( schemesep - url ) );
		if(out->scheme == 0x0)
			return 0x0;
		return &schemesep[3];
	}
}

static const char* parse_url_parse_user_pass( const char* url, parse_url_ctx* ctx, parsed_url* out )
{
	const char* atpos = strchr( url, '@' );
	if( atpos != 0x0 )
	{
		// ... check for a : before the @ ...
		const char* passsep = parse_url_strnchr( url, (size_t)( atpos - url ), ':' );
		if( passsep == 0 )
		{
			out->pass = "";
			out->user = parse_url_alloc_string( ctx, url, (size_t)( atpos - url ) );
		}
		else
		{
			size_t userlen = (size_t)(passsep - url);
			size_t passlen = (size_t)(atpos - passsep - 1);
			out->user = (char*)parse_url_alloc_string( ctx, url, userlen );
			out->pass = (char*)parse_url_alloc_string( ctx, passsep + 1, passlen );
		}

		if(out->user == 0x0 || out->pass == 0x0)
			return 0x0;

		return atpos + 1;
	}

	return url;
}

static bool parse_url_is_hex_char( char c )
{
	return (c >= 'a' && c <= 'f') ||
		   (c >= 'A' && c <= 'F') ||
		   (c >= '0' && c <= '9');
}

static char parse_url_hex_char_value( char c )
{
	if(c >= '0' && c <= '9') return c - '0';
	if(c >= 'a' && c <= 'f') return c - 'a' + 10;
	if(c >= 'A' && c <= 'F') return c - 'A' + 10;
	return 0;
}

static char* parse_url_unescape_percent_encoding( char* str )
{
	char* read  = str;
	char* write = str;

	while(*read)
	{
		if(*read == '%')
		{
			++read;
			if(!parse_url_is_hex_char(*read))
				return 0x0;
			char v1 = parse_url_hex_char_value(*read); 

			++read;
			if(!parse_url_is_hex_char(*read))
				return 0x0;

			char v2 = parse_url_hex_char_value(*read);

			*write = (char)((v1 << 4) | v2);
		}
		else
		{
			*write = *read;
		}
		++read;
		++write;
	}
	*write = '\0';
	return str;
}

static const char* parse_url_parse_host_port( const char* url, parse_url_ctx* ctx, parsed_url* out )
{
	out->port = parse_url_default_port_for_scheme( out->scheme );

	size_t hostlen = 0;
	const char* ipv6_end = 0x0;

	if(url[0] == '[')
	{
		// ipv6 host is always enclosed in a [] to handle the : in an ipv6 address.
		ipv6_end = strchr( url, ']' );
		if(ipv6_end == 0x0)
			return 0x0;
	}

	const char* portsep = strchr( ipv6_end ? ipv6_end + 1 : url, ':' );
	const char* pathsep = strchr( ipv6_end ? ipv6_end + 1 : url, '/' );

	if( portsep == 0x0 )
	{
		hostlen = pathsep == 0x0 ? strlen( url ) : (size_t)( pathsep - url );
	}
	else
	{
		if(pathsep && portsep && (pathsep < portsep))
		{
			// ... path separator was before port-separator, i.e. the : was not a port-separator! ...
			hostlen = (size_t)( pathsep - url );
		}
		else
		{
			out->port = (unsigned int)atoi( portsep + 1 );
			hostlen = (size_t)( portsep - url );
			pathsep = strchr( portsep, '/' );
		}
	}

	if( hostlen > 0 )
	{
		if(ipv6_end)
		{
			// ... we have an ipv6 host, we need to strip of the []
			out->host = parse_url_alloc_lower_string( ctx, url + 1, hostlen - 2 );

			// ... verify that the host is actually a valid ipv6 address... I guess this
			//     might miss one or two checks.
			//     this only checks that it contains numbers or hex-chars or : or .

			for(const char* c = out->host; *c; ++c)
			{
				bool valid = parse_url_is_hex_char(*c) ||
							 (*c == ':') ||
							 (*c == '.');
				if(!valid)
					return 0x0;
			}
		}
		else
			out->host = parse_url_alloc_lower_string( ctx, url, hostlen );
		if(out->host == 0x0)
			return 0x0;
	}

	// ... parse path ... TODO: extract to own function.
	if( pathsep != 0x0 )
	{
		// ... check if there are any query or fragment to parse ...
		const char* path_end = strpbrk(pathsep, "?#");

		size_t reslen = 0;
		if(path_end)
			reslen = (size_t)(path_end - pathsep);
		else
			reslen = strlen( pathsep );

		char* path = parse_url_alloc_string( ctx, pathsep, reslen );
		if(path == 0x0)
			return 0x0;

		out->path = parse_url_unescape_percent_encoding(path);
		if(out->path == 0x0)
			return 0x0;

		return pathsep + reslen;
	}

	return url;
}

static const char* parse_url_parse_query( const char* url, parse_url_ctx* ctx, parsed_url* out )
{
	// ... do we have a query? ...
	if(*url != '?')
		return url;

	// ... skip '?' ...
	++url;

	// ... find the end of the query ...
	size_t query_len = 0;

	const char* fragment_start = strchr(url, '#');
	if(fragment_start)
		query_len = (size_t)(fragment_start - url);
	else
		query_len = strlen(url);

	out->query = parse_url_alloc_string( ctx, url, query_len );
	return out->query == 0x0
				? 0x0
				: url + query_len;
}

static const char* parse_url_parse_fragment( const char* url, parse_url_ctx* ctx, parsed_url* out )
{
	// ... do we have a fragment? ...
	if(*url != '#')
		return url;

	// ... skip '#' ...
	++url;

	size_t frag_len = strlen(url);
	out->fragment = parse_url_alloc_string( ctx, url, frag_len );

	return out->fragment == 0x0
				? 0x0
				: url + frag_len;
}

#define URL_PARSE_FAIL_IF( x ) \
	if( x )                    \
	{                          \
		if( usermem == 0x0 )   \
			free( mem );       \
		return 0x0;            \
	}

URL_PARSER_LINKAGE size_t parse_url_calc_mem_usage( const char* url )
{
	return sizeof( parsed_url ) + strlen( url ) + 7; // 7 == max number of '\0' terminate
}

URL_PARSER_LINKAGE parsed_url* parse_url( const char* url, void* usermem, size_t mem_size )
{
	void* mem = usermem;
	if( mem == 0x0 )
	{
		mem_size = parse_url_calc_mem_usage( url );
		mem = malloc( mem_size );
	}

	parse_url_ctx ctx = {mem, mem_size, mem_size};

	parsed_url* out = (parsed_url*)parse_url_alloc_mem( &ctx, sizeof( parsed_url ) );
	URL_PARSE_FAIL_IF( out == 0x0 );

	// ... set default values ...
	memset(out, 0x0, sizeof(parsed_url));
	out->host = "localhost";
	out->path = "/";

	url = parse_url_parse_scheme   ( url, &ctx, out ); URL_PARSE_FAIL_IF( url == 0x0 );
	url = parse_url_parse_user_pass( url, &ctx, out ); URL_PARSE_FAIL_IF( url == 0x0 );
	url = parse_url_parse_host_port( url, &ctx, out ); URL_PARSE_FAIL_IF( url == 0x0 );
	url = parse_url_parse_query    ( url, &ctx, out ); URL_PARSE_FAIL_IF( url == 0x0 );
	url = parse_url_parse_fragment ( url, &ctx, out ); URL_PARSE_FAIL_IF( url == 0x0 );

	return out;
}
#endif // defined(URL_PARSER_IMPLEMENTATION)

#endif // URL_H_INCLUDED
