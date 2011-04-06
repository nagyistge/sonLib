/*
 * Copyright (C) 2006-2011 by Benedict Paten (benedictpaten@gmail.com)
 *
 * Released under the MIT license, see LICENSE.txt
 */

/*
 * sonLibKVDatabaseConf.c
 *
 *  Created on: 2010-08-23
 *      Author: Mark Diekhans
 */
#include "sonLibKVDatabaseConf.h"
#include "sonLibKVDatabase.h"
#include "sonLibExcept.h"
#include "sonLibTypes.h"
#include "sonLibString.h"
#include "stSafeC.h"
#include "sonLib.h"

/* object storing the database configuration */
struct stKVDatabaseConf {
    stKVDatabaseType type;
    char *databaseDir;
    char *host;
    unsigned port;
    char *user;
    char *password;
    char *databaseName;
    char *tableName;
};

stKVDatabaseConf *stKVDatabaseConf_constructTokyoCabinet(const char *databaseDir) {
    stKVDatabaseConf *conf = stSafeCCalloc(sizeof(stKVDatabaseConf));
    conf->type = stKVDatabaseTypeTokyoCabinet;
    conf->databaseDir = stString_copy(databaseDir);
    return conf;
}

static stKVDatabaseConf *constructSql(stKVDatabaseType type, const char *host, unsigned port, const char *user, const char *password,
                                      const char *databaseName, const char *tableName) {
    stKVDatabaseConf *conf = stSafeCCalloc(sizeof(stKVDatabaseConf));
    conf->type = type;
    conf->host = stString_copy(host);
    conf->port = port;
    conf->user = stString_copy(user);
    conf->password = stString_copy(password);
    conf->databaseName = stString_copy(databaseName);
    conf->tableName = stString_copy(tableName);
    return conf;
}

stKVDatabaseConf *stKVDatabaseConf_constructMySql(const char *host, unsigned port, const char *user, const char *password,
                                                  const char *databaseName, const char *tableName) {
#ifndef HAVE_MYSQL
    stThrowNew(ST_KV_DATABASE_EXCEPTION_ID,
            "requested MySQL database, however sonlib is not compiled with MySql support");
#endif
    return constructSql(stKVDatabaseTypeMySql, host, port, user, password, databaseName, tableName);
}

stKVDatabaseConf *stKVDatabaseConf_constructPostgreSql(const char *host, unsigned port, const char *user, const char *password,
                                                       const char *databaseName, const char *tableName) {
#ifndef HAVE_POSTGRESQL
    stThrowNew(ST_KV_DATABASE_EXCEPTION_ID,
            "requested PostgreSql database, however sonlib is not compiled with MySql support");
#endif
    return constructSql(stKVDatabaseTypePostgreSql, host, port, user, password, databaseName, tableName);
}

char *getNextToken(char **tokenStream) {
    char *token = stString_getNextWord(tokenStream);
    if(token != NULL) {
        char *cA = stString_replace(token, "\"", "");
        free(token);
        token = stString_replace(cA, "'", "");
        free(cA);
    }
    return token;
}

static void getExpectedToken(char **tokenStream, const char *expected) {
    char *cA = getNextToken(tokenStream);
    if (!stString_eq(cA, expected)) {
        stThrowNew(ST_KV_DATABASE_EXCEPTION_ID,
                   "BUG: expected the token: %s in database XML string, but I got: %s from the stream %s",
                   expected, cA, *tokenStream);
    }
    free(cA);
}

static char *getKeyValue(char **tokenStream, const char *key) {
    getExpectedToken(tokenStream, key);
    return getNextToken(tokenStream);
}

/* Parse XML string into a hash.  This parses all attributes of all tags
 * into values.  st_kv_database_conf type is stored as conf_type,
 * database tag is stores as db_tag.  This does minimal error checking
 * and is really lame.
 */
static stHash *hackParseXmlString(const char *xmlString) {
    stHash *hash = stHash_construct3(stHash_stringKey, stHash_stringEqualKey, free, free);
    char *toReplace[5] = { "</", "<", "/>", ">", "=" };
    char *cA = stString_replace(xmlString, toReplace[0], " "), *cA2;
    for (int32_t i = 1; i < 5; i++) {
        cA2 = stString_replace(cA, toReplace[i], " ");
        free(cA);
        cA = cA2;
    }
    getExpectedToken(&cA2, "st_kv_database_conf");
    stHash_insert(hash, stString_copy("conf_type"), getKeyValue(&cA2, "type"));
    stHash_insert(hash, stString_copy("db_tag"), getNextToken(&cA2));

    char *key;
    while (((key = getNextToken(&cA2)) != NULL) && !stString_eq(key, "st_kv_database_conf")) {
        char *value = getNextToken(&cA2);
        if (value == NULL) {
            stThrowNew(ST_KV_DATABASE_EXCEPTION_ID, "failed to to get value for key \"%s\"", key);
        }
        if (stHash_search(hash, key) != NULL) {
            stThrowNew(ST_KV_DATABASE_EXCEPTION_ID, "got a duplicate entry in the database conf string \"%s\"", key);
        }
        stHash_insert(hash, key, value);
    }
    return hash;
}

static const char *getXmlValueRequired(stHash *hash, const char *key) {
    const char *value = stHash_search(hash, (char*)key);
    if (value == NULL) {
        stThrowNew(ST_KV_DATABASE_EXCEPTION_ID, "did not find a \"%s\" value in the database XML string", key);
    }
    return value;
}

static int getXmlPort(stHash *hash) {
    const char *value = stHash_search(hash, "port");
    if (value == NULL) {
        return 0;
    } else {
        return stSafeStrToUInt32(value);
    }
}

static stKVDatabaseConf *constructFromString(const char *xmlString) {
    stHash *hash = hackParseXmlString(xmlString);
    stKVDatabaseConf *databaseConf = NULL;
    const char *type = getXmlValueRequired(hash, "conf_type");
    const char *dbTag = getXmlValueRequired(hash, "db_tag");
    if (!stString_eq(type, dbTag)) {
        stThrowNew(ST_KV_DATABASE_EXCEPTION_ID, "Database XML tag \"%s\" did not match st_kv_database_conf type attribute", dbTag, type);
    }
    if (stString_eq(type, "tokyo_cabinet")) {
        databaseConf = stKVDatabaseConf_constructTokyoCabinet(getXmlValueRequired(hash, "database_dir"));
    } else if (stString_eq(type, "mysql")) {
        databaseConf = stKVDatabaseConf_constructMySql(getXmlValueRequired(hash, "host"), getXmlPort(hash),
                                                       getXmlValueRequired(hash, "user"), getXmlValueRequired(hash, "password"),
                                                       getXmlValueRequired(hash, "database_name"), getXmlValueRequired(hash, "table_name"));
    } else if (stString_eq(type, "postgresql")) {
        databaseConf = stKVDatabaseConf_constructPostgreSql(getXmlValueRequired(hash, "host"), getXmlPort(hash),
                                                            getXmlValueRequired(hash, "user"), getXmlValueRequired(hash, "password"),
                                                            getXmlValueRequired(hash, "database_name"), getXmlValueRequired(hash, "table_name"));
    } else {
        stThrowNew(ST_KV_DATABASE_EXCEPTION_ID, "invalid database type \"%s\"", type);
    }
    stHash_destruct(hash);
    return databaseConf;
}

stKVDatabaseConf *stKVDatabaseConf_constructFromString(const char *xmlString) {
    stKVDatabaseConf *conf = NULL;
    stTry {
        conf = constructFromString(xmlString);
    } stCatch(ex) {
        stThrowNewCause(ex, ST_KV_DATABASE_EXCEPTION_ID, "Invalid database XML specification: %s", xmlString);
    } stTryEnd;
    return conf;
}


stKVDatabaseConf *stKVDatabaseConf_constructClone(stKVDatabaseConf *srcConf) {
    stKVDatabaseConf *conf = stSafeCCalloc(sizeof(stKVDatabaseConf));
    conf->type = srcConf->type;
    conf->databaseDir = stString_copy(srcConf->databaseDir);
    conf->host = stString_copy(srcConf->host);
    conf->port = srcConf->port;
    conf->user = stString_copy(srcConf->user);
    conf->password = stString_copy(srcConf->password);
    conf->databaseName = stString_copy(srcConf->databaseName);
    conf->tableName = stString_copy(srcConf->tableName);
    return conf;
}

void stKVDatabaseConf_destruct(stKVDatabaseConf *conf) {
    if (conf != NULL) {
        stSafeCFree(conf->databaseDir);
        stSafeCFree(conf->host);
        stSafeCFree(conf->user);
        stSafeCFree(conf->password);
        stSafeCFree(conf->databaseName);
        stSafeCFree(conf->tableName);
        stSafeCFree(conf);
    }
}

stKVDatabaseType stKVDatabaseConf_getType(stKVDatabaseConf *conf) {
    return conf->type;
}

const char *stKVDatabaseConf_getDir(stKVDatabaseConf *conf) {
    return conf->databaseDir;
}

const char *stKVDatabaseConf_getHost(stKVDatabaseConf *conf) {
    return conf->host;
}

unsigned stKVDatabaseConf_getPort(stKVDatabaseConf *conf) {
    return conf->port;
}

const char *stKVDatabaseConf_getUser(stKVDatabaseConf *conf) {
    return conf->user;
}

const char *stKVDatabaseConf_getPassword(stKVDatabaseConf *conf) {
    return conf->password;
}

const char *stKVDatabaseConf_getDatabaseName(stKVDatabaseConf *conf) {
    return conf->databaseName;
}

const char *stKVDatabaseConf_getTableName(stKVDatabaseConf *conf) {
    return conf->tableName;
}
