/*
 ============================================================================
 Name        : rsf.c
 Author      : Qiniu.com
 Copyright   : 2017(c) Shanghai Qiniu Information Technologies Co., Ltd.
 Description : list files from qiniu bucket
 ============================================================================
 */

#include "rsf.h"
#include <stdio.h>
#include "../cJSON/cJSON.h"

Qiniu_Error Qiniu_RSF_ListFiles(Qiniu_Client *self, Qiniu_RSF_ListRet *ret, const char *bucket, const char *prefix,
                                const char *delimiter, const char *marker, int limit) {
    Qiniu_Error err;
    int i;
    int prefixCount;
    int itemCount;

    if (limit < 1 || limit > 1000) {
        err.code = 400;
        err.message = "invalid list limit";
        return err;
    }

    cJSON *root;
    char *encodedBucket = 0;
    char *encodedMarker = 0;
    char *encodedPrefix = 0;
    char *encodedDelimiter = 0;

    char *url;

    Qiniu_Bool escapeBucketOk;
    Qiniu_Bool escapePrefixOk;
    Qiniu_Bool escapeDelimiterOk;
    Qiniu_Bool escapeMarkerOk;

    encodedBucket = Qiniu_QueryEscape(bucket, &escapeBucketOk);

    if (prefix) {
        encodedPrefix = Qiniu_QueryEscape(prefix, &escapePrefixOk);
    } else {
        encodedPrefix = "";
    }

    if (marker) {
        encodedMarker = Qiniu_QueryEscape(marker, &escapeMarkerOk);
    } else {
        encodedMarker = "";
    }

    if (delimiter) {
        encodedDelimiter = Qiniu_QueryEscape(delimiter, &escapeDelimiterOk);
    } else {
        encodedDelimiter = "";
    }

    size_t limitLen = snprintf(0, 0, "%d", limit) + 1;
    char *limitStr = (char *) malloc(sizeof(char) * limitLen);
    memset(limitStr, 0, limitLen);
    snprintf(limitStr, limitLen, "%d", limit);

    url = Qiniu_String_Concat(QINIU_RSF_HOST, "/list?", "bucket=", encodedBucket, "&prefix=", encodedPrefix,
                              "&delimiter=", encodedDelimiter, "&marker=", encodedMarker, "&limit=", limitStr, 0);
    Qiniu_Free(limitStr);
    if (escapeBucketOk) {
        Qiniu_Free(encodedBucket);
    }

    if (prefix && escapePrefixOk) {
        Qiniu_Free(encodedPrefix);
    }

    if (delimiter && escapeDelimiterOk) {
        Qiniu_Free(encodedDelimiter);
    }

    if (marker && escapeMarkerOk) {
        Qiniu_Free(encodedMarker);
    }

    err = Qiniu_Client_Call(self, &root, url);
    Qiniu_Free(url);
    if (err.code == 200) {
        ret->marker = Qiniu_Json_GetString(root, "marker", 0);
        //check common prefixes
        prefixCount = Qiniu_Json_GetArraySize(root, "commonPrefixes", 0);
        if (prefixCount > 0) {
            ret->commonPrefix = (Qiniu_RSF_CommonPrefix *) malloc(sizeof(Qiniu_RSF_CommonPrefix) * prefixCount);
            Qiniu_Json *commonPrefixes = Qiniu_Json_GetObjectItem(root, "commonPrefixes", 0);
            Qiniu_RSF_CommonPrefix *nextCommonPrefix = 0;

            for (i = prefixCount - 1; i >= 0; i--) {
                char *commonPrefix = strdup(Qiniu_Json_GetArrayItem(commonPrefixes, i, 0)->valuestring);
                Qiniu_RSF_CommonPrefix *commonPrefixElem = (Qiniu_RSF_CommonPrefix *) malloc(
                        sizeof(Qiniu_RSF_CommonPrefix));
                commonPrefixElem->value = commonPrefix;
                commonPrefixElem->next = nextCommonPrefix;
                ret->commonPrefix[i] = *commonPrefixElem;
                nextCommonPrefix = commonPrefixElem;
            }
        } else {
            ret->commonPrefix = 0;
        }
        //check items
        itemCount = Qiniu_Json_GetArraySize(root, "items", 0);
        if (itemCount > 0) {
            ret->item = (Qiniu_RSF_ListItem *) malloc(sizeof(Qiniu_RSF_ListItem) * itemCount);
            Qiniu_Json *items = Qiniu_Json_GetObjectItem(root, "items", 0);
            Qiniu_RSF_ListItem *nextItem = 0;
            for (i = itemCount - 1; i >= 0; i--) {
                Qiniu_Json *itemObj = Qiniu_Json_GetArrayItem(items, i, 0);
                Qiniu_RSF_ListItem *item = (Qiniu_RSF_ListItem *) malloc(sizeof(Qiniu_RSF_ListItem));
                item->key = strdup(Qiniu_Json_GetString(itemObj, "key", 0));
                item->hash = strdup(Qiniu_Json_GetString(itemObj, "hash", 0));
                item->mimeType = strdup(Qiniu_Json_GetString(itemObj, "mimeType", 0));
                if (Qiniu_Json_GetString(itemObj, "endUser", 0) != 0) {
                    item->endUser = strdup(Qiniu_Json_GetString(itemObj, "endUser", 0));
                } else {
                    item->endUser = 0;
                }
                item->putTime = Qiniu_Json_GetInt64(itemObj, "putTime", 0);
                item->fsize = Qiniu_Json_GetInt64(itemObj, "fsize", 0);
                item->type = Qiniu_Json_GetInt64(itemObj, "type", 0);
                item->next = nextItem;
                ret->item[i] = *item;
                nextItem = item;
            }
        } else {
            ret->item = 0;
        }
    }

    return err;
}