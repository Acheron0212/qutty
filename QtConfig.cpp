#define QUTTY_XMLIZE_STRUCTS

#include "QtConfig.h"
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QDir>
#include <QFile>
#include <QObject>
#include <QMessageBox>
#include <QDebug>

QtConfig::QtConfig()
{
}

int QtConfig::readFromXML(QIODevice *device)
{
    QXmlStreamReader xml;
    int i, j;
    char *tmpbuf;
    int tmplen;

    xml.setDevice(device);
    if (!xml.readNextStartElement() || xml.name() != "qutty" ||
            xml.attributes().value("version") != "1.0") {
        QMessageBox::warning(NULL, QObject::tr("Qutty Configuration"),
                             QObject::tr("Invalid xml file"));
        return false;
    }
    while (xml.readNextStartElement()) {
        if (xml.name() == "config" && xml.attributes().value("version") == "1.0") {
            Config cfg;
            memset(&cfg, 0, sizeof(Config));
            while (xml.readNextStartElement()) {
                if (xml.name() == "dataelement") {
                    QStringRef tmptype = xml.attributes().value("datatype");
                    QStringRef tmpname = xml.attributes().value("dataname");
                    QStringRef tmpqstr = xml.attributes().value("datavalue");
                    QByteArray tmpbarr = tmpqstr.toLocal8Bit();
                    tmpbuf = tmpbarr.data();
#define int(a) if (tmptype=="int"  && tmpname==#a) sscanf(tmpbuf, "%d", &cfg.a);
#define Filename(a) if (tmptype=="Filename"  && tmpname==#a) \
            sscanf(tmpbuf, "%s", cfg.a.path);
#define FontSpec(a) if (tmptype=="FontSpec"  && tmpname==#a) \
            sscanf(tmpbuf, "%d %d %d %s", \
                &cfg.a.isbold, &cfg.a.height, &cfg.a.charset, cfg.a.name);
#define QUTTY_SERIALIZE_ELEMENT_ARRAY_int(name, arr) \
            if (tmpname==#name) \
                for(i=0; i<arr; i++) \
                    sscanf(tmpbuf+i*9, "%08X ", &cfg.name[i]);
#define QUTTY_SERIALIZE_ELEMENT_ARRAY_short(name, arr) \
            QUTTY_SERIALIZE_ELEMENT_ARRAY_int(name, arr)
#define QUTTY_SERIALIZE_ELEMENT_ARRAY_char(name, arr) \
            if (tmpname==#name) { \
                strncpy(cfg.name, tmpbuf, sizeof(cfg.name)); \
            }
#define QUTTY_SERIALIZE_ELEMENT_ARRAY(t, n, a) QUTTY_SERIALIZE_ELEMENT_ARRAY_##t(n, a);

            QUTTY_SERIALIZE_STRUCT_CONFIG_ELEMENT_LIST

            if (tmptype == "unsigned char" && tmpname == "colours")
                for(i=0; i<22; i++) {
                    int a, b, c;
                    sscanf(tmpbuf+i*9, "%X %X %X ", &a, &b, &c);
                    cfg.colours[i][0] = (uchar)a;
                    cfg.colours[i][1] = (uchar)b;
                    cfg.colours[i][2] = (uchar)c;
                }
#undef QUTTY_SERIALIZE_ELEMENT_ARRAY_short
#undef QUTTY_SERIALIZE_ELEMENT_ARRAY_int
#undef QUTTY_SERIALIZE_ELEMENT_ARRAY_char
#undef QUTTY_SERIALIZE_ELEMENT_ARRAY
#undef int
#undef Filename
#undef FontSpec
                    xml.skipCurrentElement();
                } else {
                    xml.skipCurrentElement();
                }
            }
            qutty_config.config_list[cfg.config_name] = cfg;
        } else {
            xml.skipCurrentElement();
        }
    }
    return 0;
}

int QtConfig::writeToXML(QIODevice *device)
{
    QXmlStreamWriter xml;
    char tmpbuf[10240];
    int tmplen;
    xml.setDevice(device);
    xml.setAutoFormatting(true);
    map<string, Config>::iterator it;

    xml.writeStartDocument();
    xml.writeDTD("<!DOCTYPE qutty>");
    xml.writeStartElement("qutty");
    xml.writeAttribute("version", "1.0");
    for(it=config_list.begin(); it != config_list.end(); it++) {
        int i, j;
        Config *cfg = &(it->second);

        xml.writeStartElement("config");
        xml.writeAttribute("version", "1.0");

#define XMLIZE(type, name, value) \
    xml.writeStartElement("dataelement"); \
    xml.writeAttribute("datatype", type); \
    xml.writeAttribute("dataname", name); \
    xml.writeAttribute("datavalue", value); \
    xml.writeEndElement();
#define int(a) _snprintf(tmpbuf, sizeof(tmpbuf), "%d", cfg->a); XMLIZE("int", #a, tmpbuf);
#define Filename(a) \
        _snprintf(tmpbuf, sizeof(tmpbuf), "%s", cfg->a.path); \
        XMLIZE("Filename", #a, tmpbuf);
#define FontSpec(a) \
        _snprintf(tmpbuf, sizeof(tmpbuf), "%d %d %d %s", \
                cfg->a.isbold, cfg->a.height, cfg->a.charset, cfg->a.name); \
        XMLIZE("FontSpec", #a, tmpbuf);
#define QUTTY_SERIALIZE_ELEMENT_ARRAY_int(type, name, arr) \
        tmplen = 0; \
        for(i=0; i<arr; i++) \
            tmplen += _snprintf(tmpbuf+tmplen, sizeof(tmpbuf)-tmplen, \
                                    "%08X ", cfg->name[i]); \
        XMLIZE(#type, #name, tmpbuf);
#define QUTTY_SERIALIZE_ELEMENT_ARRAY_short(type, name, arr) \
        QUTTY_SERIALIZE_ELEMENT_ARRAY_int(type, name, arr);
#define QUTTY_SERIALIZE_ELEMENT_ARRAY_char(type, name, arr) \
        _snprintf(tmpbuf, sizeof(tmpbuf), "%s", cfg->name); \
        XMLIZE(#type, #name, tmpbuf);
#define QUTTY_SERIALIZE_ELEMENT_ARRAY(t, n, a) QUTTY_SERIALIZE_ELEMENT_ARRAY_##t(t, n, a);

        QUTTY_SERIALIZE_STRUCT_CONFIG_ELEMENT_LIST;

        for(i=0, tmplen=0; i<22; i++)
            tmplen += _snprintf(tmpbuf+tmplen, sizeof(tmpbuf)-tmplen, "%02X %02X %02X ",
                                cfg->colours[i][0], cfg->colours[i][1], cfg->colours[i][2]);
        XMLIZE("unsigned char", "colours", tmpbuf);
#undef QUTTY_SERIALIZE_ELEMENT_ARRAY_short
#undef QUTTY_SERIALIZE_ELEMENT_ARRAY_int
#undef QUTTY_SERIALIZE_ELEMENT_ARRAY_char
#undef QUTTY_SERIALIZE_ELEMENT_ARRAY
#undef int
#undef Filename
#undef FontSpec
        xml.writeEndElement();
    }

    xml.writeEndDocument();
    return 0;
}

bool QtConfig::restoreConfig()
{
    config_list.clear();
    QFile file(QDir::home().filePath("qutty.xml"));
    if (!file.exists()) {
        Config cfg;
        initConfigDefaults(&cfg);
        strcpy(cfg.config_name, QUTTY_DEFAULT_CONFIG_SETTINGS);
        qutty_config.config_list[QUTTY_DEFAULT_CONFIG_SETTINGS] = cfg;
        saveConfig();
    }
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(NULL, QObject::tr("Qutty Configuration"),
                             QObject::tr("Cannot read file %1:\n%2.")
                             .arg(file.fileName())
                             .arg(file.errorString()));
        return false;
    }
    readFromXML(&file);
    return true;
}

bool QtConfig::saveConfig()
{
    QFile file(QDir::home().filePath("qutty.xml"));
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(NULL, QObject::tr("Qutty Configuration"),
                             QObject::tr("Cannot write file %1:\n%2.")
                             .arg(file.fileName())
                             .arg(file.errorString()));
        return false;
    }
    writeToXML(&file);
    return true;
}