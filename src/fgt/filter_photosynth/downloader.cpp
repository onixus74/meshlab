#include "synthData.h"
#include <QtSoapMessage>

/*************
 * SynthData *
 *************/

SynthData::SynthData(QObject *parent)
  : QObject(parent)
{
  _coordinateSystems = new QList<CoordinateSystem>();
}

SynthData::SynthData(const SynthData &other)
{
  _collectionID = other._collectionID;
  _collectionRoot = other._collectionRoot;
  _coordinateSystems = new QList<CoordinateSystem>(*other._coordinateSystems);
}

SynthData::~SynthData()
{
  delete _coordinateSystems;
}

void SynthData::setCollectionID(QString id)
{
  _collectionID = id;
}

void SynthData::setCollectionRoot(QString collectionRoot)
{
  _collectionRoot = collectionRoot;
}

bool SynthData::isValid()
{
  bool valid = false;
  if(!_collectionID.isNull() && !_collectionID.isEmpty() && !_collectionRoot.isNull() && !_collectionRoot.isEmpty())
      valid = true;
  return valid;
}

QtSoapHttpTransport SynthData::transport;

SynthData *SynthData::downloadSynthInfo(QString url)
{
  SynthData *synthData = new SynthData();

  if(url.isNull() || url.isEmpty())
    return synthData;

  int i = url.indexOf("cid=",0,Qt::CaseInsensitive);
  if(i < 0 || url.length() < i + 40)
    return synthData;

  ImportSettings::ImportSource importSource = ImportSettings::WEB_SITE;
  QString cid = url.mid(i + 4, 36);
  bool importPointClouds = true;
  bool importCameraParameters = false;
  ImportSettings settings(importSource,cid,importPointClouds,importCameraParameters);
  synthData->setCollectionID(cid);

  if(settings.source() == ImportSettings::WEB_SITE)
  {
    QtSoapMessage message;
    message.setMethod("GetCollectionData", "http://labs.live.com/");
    message.addMethodArgument("collectionId", "", settings.sourcePath());
    message.addMethodArgument("incrementEmbedCount", "", false, 0);

    transport.setAction("http://labs.live.com/GetCollectionData");
    transport.setHost("photosynth.net");
    QObject::connect(&transport, SIGNAL(responseReady()), synthData, SLOT(readWSresponse()));

    transport.submitRequest(message, "/photosynthws/PhotosynthService.asmx");
  }

  return synthData;
}

void SynthData::readWSresponse()
{
  const QtSoapMessage &response = transport.getResponse();
  if(response.isFault())
  {
    qWarning("ERROR: %s\n",response.faultString().toString().toLatin1().constData());
    return;
  }

#ifdef FILTER_PHOTOSYNTH_DEBUG_SOAP
  printf("-----------RESPONSE------------\n");
  qWarning("%s\n", response.toXmlString().toLatin1().constData());
  printf("-----------------------\n");
#endif

  const QtSoapType &returnValue = response.returnValue();
  if (returnValue["Result"].isValid())
    if(returnValue["Result"].toString() == "OK")
    {
      if(returnValue["CollectionType"].toString() == "Synth")
      {
        QString jsonURL = returnValue["JsonUrl"].toString();
        QString jsonString = downloadJsonData(jsonURL);
        if(jsonString.isEmpty())
        {
          qWarning("Could not download Json data.\n");
          return;
        }
        if(!this->parseJsonString(jsonString))
          return;
        this->_collectionRoot = returnValue["CollectionRoot"].toString();
      }
      else
      {
        qWarning("This filter is compatible with photosynths only, the provided url belongs to the category: %s\n",returnValue["CollectionType"].toString().toLatin1().constData());
        return;
      }
    }
    else
    {
      qWarning("The web service returned: %s\n",returnValue["Result"].toString().toLatin1().constData());
      return;
    }
  else
    qWarning("Cannot read the response\n");
}

QString SynthData::downloadJsonData(QString jsonURL)
{
  QString str;
  return str;
}

bool SynthData::parseJsonString(QString jsonString)
{
  return false;
}

/******************
 * ImportSettings *
 ******************/

ImportSettings::ImportSettings(ImportSource source, QString sourcePath, bool importPointClouds, bool importCameraParameters)
{
  _source = source;
  _sourcePath = sourcePath;
  _importPointClouds = importPointClouds;
  _importCameraParameters = importCameraParameters;
}

ImportSettings::ImportSource ImportSettings::source()
{
  return _source;
}

QString ImportSettings::sourcePath()
{
  return _sourcePath;
}

bool ImportSettings::importPointClouds()
{
  return _importPointClouds;
}

bool ImportSettings::importCameraParameters()
{
  return _importCameraParameters;
}
