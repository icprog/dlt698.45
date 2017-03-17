/*
 * lib698def.h
 *
 *  Created on: Jan 6, 2017
 *      Author: lhl
 */

#ifndef LIB698DEF_H_
#define LIB698DEF_H_



#define LINK_REQUEST   1
#define LINK_RESPONSE  129
#define CONNECT_REQUEST  2
#define RELEASE_REQUEST  3
#define GET_REQUEST  5
#define GET_REQUEST_RESPONSE  133
		#define GET_REQUEST_NORMAL	1
		#define GET_REQUEST_NORMAL_LIST 2
		#define GET_REQUEST_RECORD 3
		#define GET_REQUEST_RECORD_LIST 4
		#define GET_REQUEST_RECORD_NEXT 5
#define SET_REQUEST  6
		#define SET_REQUEST_NORMAL	1
		#define SET_REQUEST_NORMAL_LIST	2
		#define SET_THENGET_REQUEST_NORMAL_LIST 3

#define ACTION_REQUEST  7
		#define ACTIONREQUEST 1
		#define ACTIONREQUEST_LIST	2
		#define ACTIONTHENGET_REQUEST_NORMAL_LIST 3

#define REPORT_RESPONSE  8
#define PROXY_REQUEST  9
		#define ProxyGetRequestList		1
		#define ProxyGetRequestRecord	2
		#define ProxySetRequestList		3
		#define ProxySetThenGetRequestList 4
		#define ProxyActionRequestList 5
		#define ProxyActionThenGetRequestList 6
		#define ProxyTransCommandRequest 7

#define CONNECT_RESPONSE 130
#define RELEASE_RESPONSE 131
#define	RELEASE_NOTIFICATION 132
#define	GET_RESPONSE 133

#define	SET_RESPONSE 134
#define	ACTION_RESPONSE 135
		#define	ActionResponseNormal			1
		#define	ActionResponseNormalList		2
		#define	ActionThenGetResponseNormalList	3
#define	REPORT_NOTIFICATION 136
        #define REPORTNOTIFICATIONLIST 1
        #define REPROTNOTIFICATIONRECORDLIST 2
#define	PROXY_RESPONSE 137
		#define ProxyGetResponseList		1
		#define ProxyGetResponseRecord	2
		#define ProxySetResponseList		3
		#define ProxySetThenGetResponseList 4
		#define ProxyActionResponseList 5
		#define ProxyActionThenGetResponseList 6
		#define ProxyTransCommandResponse 7

#define SECURITY_REQUEST 16
#define SECURITY_RESPONSE 144

#endif /* LIB698DEF_H_ */
