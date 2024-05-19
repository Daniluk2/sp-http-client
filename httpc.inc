/**
* Simple HTTP client
* Author: Daniluk2 & catalyst
* Date: 20.05.2024
*/
#if defined __HTTP_CLIENT
#endinput
#endif
#define __HTTP_CLIENT

#include <socket>

/* common functions */
stock strcopy_from(String:dst[], dstlen, String:src[], pos_from)
{
	new i;
	for(i = 0; src[i + pos_from] && i+1 < dstlen; i++)
		dst[i] = src[i + pos_from];
	
	dst[i] = src[i + pos_from];
}

stock strchr(String:str[], sym)
{
	for(new i = 0; str[i]; i++)
		if(str[i] == sym)
			return i;

	return -1;
}

/* internal callback function DO NOT USE IT! */
public on_socket_connected_internal(Handle:socket, any:h_httpc)
{
	// socket is connected, send the http request
	decl String:requestStr[1024];
	decl String:hostname[128];
	decl String:get_req[128];
	decl String:user_agent[128];
	
	ResetPack(h_httpc);
	ReadPackCell(h_httpc); //skip socket unused
	ReadPackCell(h_httpc); //skip userdata unused
	ReadPackString(h_httpc, hostname, sizeof(hostname));
	ReadPackString(h_httpc, get_req, sizeof(get_req));
	ReadPackString(h_httpc, user_agent, sizeof(user_agent));
	
	Format(requestStr, sizeof(requestStr), 
	"GET /%s HTTP/1.0\r\n"...
	"Host: %s\r\n"...
	"User-Agent: %s\r\n"...
	"Connection: close\r\n\r\n", get_req, hostname, user_agent);
	SocketSend(socket, requestStr);
}


/**
* @brief Open HTTP client
* 
* @param h_httpc - reference to dest handle
* @param host_str - hostname etc. 'google.com'
* @param port - dest port etc. 80
* @param get_req_string - get request string  etc. "/index.html"
* @param useragent_str - user agent etc. "Mozilla/5.0 (Windows 10) x64"
* @param error_cb - socket default error callback
* @param recv_cb - socket default receive callback
* @param disconnect_cb - socket default disconnect callback
* @param param - user pointer for reading from http client
* 
* @return 
*/
stock bool:httpc_open_ex(&Handle:h_httpc,
	String:host_str[],
	port,
	String:get_req_string[],
	String:useragent_str[],
	SocketErrorCB error_cb,
	SocketReceiveCB recv_cb,
	SocketDisconnectCB disconnect_cb,
	any:param = 0)
{
	new Handle:h_socket;
	new Handle:h_pack;
	
	h_socket = SocketCreate(SOCKET_TCP, error_cb);
	if(h_socket == INVALID_HANDLE)
		return false;
	
	/* create data pack */
	h_pack = CreateDataPack();
	WritePackCell(h_pack, h_socket);
	WritePackCell(h_pack, param);
	WritePackString(h_pack, host_str);
	WritePackString(h_pack, get_req_string);
	WritePackString(h_pack, useragent_str);
	SocketSetArg(h_socket, h_pack);
	SocketConnect(h_socket, on_socket_connected_internal, recv_cb, disconnect_cb, host_str, port);
	h_httpc = h_pack;
	return true;
}

/* NOT WORKING  FIX URL PARSER */
stock bool:httpc_open(&Handle:h_httpc,
	String:url[],
	port,
	String:useragent_str[],
	SocketErrorCB error_cb,
	SocketReceiveCB recv_cb,
	SocketDisconnectCB disconnect_cb,
	any:param = 0
)
{
	new String:url_copy[1024];
	new String:host[128], String:request[512];
	
	/* copy url string */
	strcopy(url_copy, sizeof(url_copy), url);
	
	/* remove 'http://' */
	ReplaceString(url_copy, sizeof(url_copy), "http://", "");
	
	new delim_first_idx = strchr(url_copy, '/');
	if(delim_first_idx == -1) {
		PrintToServer("not found delimeter");
		return false;
	}

	/* copy request url */
	url[delim_first_idx] = '\0';
	delim_first_idx++;
	strcopy_from(request, sizeof(request), url_copy, delim_first_idx);
	
	/* copy host */
	strcopy(host, sizeof(host), url_copy); //TODO: copied with '/request'
	
	/* debug strings */
	PrintToServer("\n\nhost: %s", host);
	PrintToServer("request: %s\n\n", request);
	return httpc_open_ex(h_httpc, host, port, request, useragent_str, error_cb, recv_cb, disconnect_cb, param);
}

stock bool:httpc_close(Handle:h_httpc)
{
	if(h_httpc == INVALID_HANDLE)
		return false;
	
	ResetPack(h_httpc);
	new Handle:h_socket = ReadPackCell(h_httpc);
	if(!SocketIsConnected(h_socket))
		SocketDisconnect(h_socket);

	CloseHandle(h_socket);
	
	CloseHandle(h_httpc);
	return true;
}

// ============================== EXAMPLE USE ==============================

#if 0
new Handle:h_http_client = INVALID_HANDLE;

public OnPluginStart()
{
	RegConsoleCmd("http", http_request, "");
	RegConsoleCmd("http2", http_request2, "");
}

public Action:http_request2(client, argc)
{
	new String:url[1024];
	GetCmdArgString(url, sizeof(url));
	httpc_open_ex(h_http_client, 
		"google.com",
		80,
		url,
		"GOVNO",
		OnSocketError,
		OnSocketReceive,
		OnSocketDisconnected
	);
	return Plugin_Continue;
}

public Action:http_request(client, argc)
{
	new String:url[1024];
	GetCmdArgString(url, sizeof(url));	
	if(h_http_client != INVALID_HANDLE) {
		PrintToServer("connection already active. closing...");
		httpc_close(h_http_client);
		h_http_client = INVALID_HANDLE;
	}

	if(!httpc_open(h_http_client, url, 80, "invalUd", OnSocketError, OnSocketReceive, OnSocketDisconnected)) {
		PrintToServer("failed to create socket\n");
	}
	return Plugin_Continue;
}

public OnSocketReceive(Handle:socket, String:receiveData[], const dataSize, any:h_httpc)
{
	// receive another chunk and write it to <modfolder>/dl.htm
	// we could strip the http response header here, but for example's sake we'll leave it in

	PrintToServer("\n\n\n%s\n\n\nDATA SIZE: %d", receiveData, dataSize);
}

public OnSocketDisconnected(Handle:socket, any:h_httpc)
{
	PrintToServer("OnSocketDisconnected: socket disconnected");
	httpc_close(h_httpc);
}

public OnSocketError(Handle:socket, const errorType, const errorNum, any:h_httpc)
{
	PrintToServer("OnSocketError: socket error %d (errno %d)", errorType, errorNum);
	httpc_close(h_httpc);
}
#endif