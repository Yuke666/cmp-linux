#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <openssl/crypto.h>
#include <openssl/md5.h>
#include <openssl/ssl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "songinfo.h"
#include "config.h"
#include "ui.h"

static const char *apiKey = "4df0d7f6bb3060463080f7ee62ca839d";
static const char *secret = "6133b672f1cfed6f8df36f8974e81e7b";

// static const char *apiKey = "0ff2b042b54a23f70b25951ca71c2259";
// static const char *secret = "1d68915170787f822c3646271cb482f7";

static struct addrinfo *lastfmAddr, *lastfmAddrHttps;
static int initualized = 0, authenticated = 0;


static struct {
	char name[1024];
	char key[35];
} usersLastFM;

static void do_md5(const unsigned char *in, char *out){
	unsigned char str[16] = {0};
	MD5(in, strlen((char*)in), str );
	int k;
	for(k = 0; k < 16; k++) sprintf(&out[k*2], "%02x", str[k] );
	out[32] = 0;
}

static int XML_findTagPos(const char *string, const char *cmp){

	int inTag = 0;
	
	char tag[2048];
	char *match = tag;
	
	int k;
	for (k = 0; k < strlen(string); k++){
		if(string[k]=='<') {
			inTag = 1;
			continue;
		}
		if(string[k]=='>'){
			if(strcmp(tag, cmp) == 0) return k-strlen(cmp);
			inTag = 0;
			memset(tag, 0, sizeof(tag));
			match = tag;
			continue;
		}

		if(inTag){
			*match++ = string[k];
		}
	}

	return 0;
}

static void XML_GetStrBetweenTags(const char *tag, const char *data, char *result){
	
	int s = XML_findTagPos(data, tag);

	char tag2[strlen(tag)+1];
	sprintf(tag2, "/%s", tag);
	
	int e = XML_findTagPos(data, tag2);

	s+=strlen(tag)+1;
	e-= 1;

	int j;
	for(j = s; j < e; j++)
		result[j-s] = data[j];

	result[e-s] = 0;
}

static void HTML_EscapeStr(char *str, char *out){

	int outIndex = 0;

	int k;
	for(k = 0; k < strlen(str); k++){
		if(str[k] == ' ') { out[outIndex++] = '+'; continue; }
		else if(str[k] == '!')  memcpy(&out[outIndex], "%21", 3);
		else if(str[k] == '#')  memcpy(&out[outIndex], "%23", 3);
		else if(str[k] == '$')  memcpy(&out[outIndex], "%24", 3);
		else if(str[k] == '&')  memcpy(&out[outIndex], "%26", 3);
		else if(str[k] == '\'') memcpy(&out[outIndex], "%27", 3);
		else if(str[k] == '(')  memcpy(&out[outIndex], "%28", 3);
		else if(str[k] == ')')  memcpy(&out[outIndex], "%29", 3);
		else if(str[k] == '*')  memcpy(&out[outIndex], "%2A", 3);
		else if(str[k] == '+')  memcpy(&out[outIndex], "%2B", 3);
		else if(str[k] == ',')  memcpy(&out[outIndex], "%2C", 3);
		else if(str[k] == '/')  memcpy(&out[outIndex], "%2F", 3);
		else if(str[k] == ':')  memcpy(&out[outIndex], "%3A", 3);
		else if(str[k] == ';')  memcpy(&out[outIndex], "%3B", 3);
		else if(str[k] == '=')  memcpy(&out[outIndex], "%3D", 3);
		else if(str[k] == '?')  memcpy(&out[outIndex], "%3F", 3);
		else if(str[k] == '@')  memcpy(&out[outIndex], "%40", 3);
		else if(str[k] == '[')  memcpy(&out[outIndex], "%5B", 3);
		else if(str[k] == ']')  memcpy(&out[outIndex], "%5D", 3);
		else { out[outIndex++] = str[k]; continue; }
		outIndex+=3;
	}

	out[outIndex] = 0;
}

static void GetPostRequest(char *request, char *data, char *out){
	strcat(out, request);
	strcat(out, "Host: ws.audioscrobbler.com\r\n");
	strcat(out, "User-Agent: cmp\r\n");
	strcat(out, "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n");
	strcat(out, "Accept-Language: en-US,en;q=0.5\r\n");
	strcat(out, "Connection: keep-alive\r\n");
	strcat(out, "Cache-Control: max-age=0\r\n");
	sprintf(&out[strlen(out)], "Content-Length: %i\r\n", (int)strlen(data));
	strcat(out, "\r\n");
	strcat(out, data);
}

void LastFM_Authenticate(char *username, char *password){

	if(!initualized) return;

	SSL_CTX *ctx;
	SSL *ssl;
	int sockssl = socket(AF_INET, SOCK_STREAM, 0);
	if(connect(sockssl, lastfmAddrHttps->ai_addr, lastfmAddrHttps->ai_addrlen) == 0){
		SSL_library_init();
		SSL_load_error_strings();

		ctx = SSL_CTX_new(SSLv23_method());
		ssl = SSL_new(ctx);

		SSL_set_fd(ssl, sockssl);
		SSL_connect(ssl);

		char sig[35] = {0};
		char sigBuffer[1024] = {0};
		sprintf(sigBuffer, "api_key%s"
			"methodauth.getMobileSessionpassword%susername%s%s",
		 apiKey,password, username, secret);

		do_md5((unsigned char*)sigBuffer, sig);

		char request[2048];
		sprintf(request, "POST /2.0/?method=auth.getMobileSession&api_key=%s"
			"&username=%s&password=%s&api_sig=%s HTTP/1.1\r\n", 
			apiKey, username, password, sig);

		char buf[4096] = {0};
		GetPostRequest(request,"\0", buf);

		SSL_write(ssl, buf, strlen(buf));

		char result[2048];
		SSL_read(ssl, result, 1024);
	    
	    FILE *fp = fopen("log.txt","w");
	    fwrite(result, 1, strlen(result), fp);
	    fclose(fp);

		XML_GetStrBetweenTags("name",result,usersLastFM.name);
		XML_GetStrBetweenTags("key",result,usersLastFM.key);

		if(strlen(usersLastFM.key) > 0)
			authenticated = 1;


		SSL_shutdown(ssl);
		SSL_free(ssl);
		SSL_CTX_free(ctx);
	}
}

void LastFM_ScrobbleTrack(SongInfo song, time_t startedTime){

	if(!authenticated) return;

	int sockssl = socket(AF_INET, SOCK_STREAM, 0);
	if(connect(sockssl, lastfmAddr->ai_addr, lastfmAddr->ai_addrlen) == 0){

		char trackName[128];
		char trackArtist[128];
		HTML_EscapeStr(song.title, trackName);
		HTML_EscapeStr(song.artist, trackArtist);

		char timestamp[32];
		sprintf(timestamp, "%i", (int)startedTime);

		char sig[35] = {0};
		char sigBuffer[1024] = {0};
		// sprintf(sigBuffer, "api_key%sartist%s"
		// 	"methodtrack.lovesk%strack%s%s",
		//  apiKey, song.artist,usersLastFM.key, song.title, secret);
		sprintf(sigBuffer, "api_key%sartist%s"
			"methodtrack.scrobblesk%stimestamp%strack%s%s",
		 apiKey, song.artist,usersLastFM.key, timestamp, song.title, secret);

		do_md5((unsigned char*)sigBuffer, sig);

		char request[2048];
		sprintf(request, "POST /2.0/?method=track.scrobble&api_key=%s"
			"&track=%s&artist=%s&sk=%s&timestamp=%s&api_sig=%s HTTP/1.1\r\n", 
			apiKey, trackName, trackArtist,usersLastFM.key, timestamp, sig);
		// sprintf(request, "POST /2.0/?method=track.love&api_key=%s"
		// 	"&track=%s&artist=%s&sk=%s&api_sig=%s HTTP/1.1\r\n", 
		// 	apiKey, trackName, trackArtist,usersLastFM.key, sig);

		char buf[4096] = {0};
		GetPostRequest(request,"\0", buf);

	    FILE *fp = fopen("log.txt","w");
	    fwrite(buf, 1, strlen(buf), fp);
	    fclose(fp);

		if(send(sockssl, buf, strlen(buf),0) <= 0){
		    FILE *fp = fopen("log.txt","w");
		    fwrite("error", 1, strlen("error"), fp);
		    fclose(fp);
		}

		char result[2048];
		if(recv(sockssl, result, 1024,0) > 0){

			FILE *fp = fopen("log1.txt","w");
			fwrite(result, 1, strlen(result), fp);
			fclose(fp);
		}

		// SSL_shutdown(ssl);
		// SSL_free(ssl);
		// SSL_CTX_free(ctx);
		close(sockssl);
	}
}

void LastFM_Init(){
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;
	getaddrinfo("ws.audioscrobbler.com", "80", &hints, &lastfmAddr );
	getaddrinfo("ws.audioscrobbler.com", "443", &hints, &lastfmAddrHttps );
	initualized = 1;

		FILE *fp = fopen("log.txt","w");
		fclose(fp);
		fp = fopen("log1.txt","w");
		fclose(fp);


}

void LastFM_Close(){
	if(!initualized) return;
	freeaddrinfo(lastfmAddr);
	freeaddrinfo(lastfmAddrHttps);
}