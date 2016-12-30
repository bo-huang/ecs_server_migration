#include "migration.h"
#include <fstream>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <CloudSDK/aliyunclient.h>
#include <CloudSDK/azureclient.h>
#include <CloudSDK/baiduyunclient.h>
#include <CloudSDK/googleclient.h>
#include <CloudSDK/s3client.h>
#include <QEventLoop>
#include <QNetworkReply>
#include <stdio.h>

Migration::Migration(QString directory,int interval)
    :rootPath(directory),interval(interval)
{
    manger = new QNetworkAccessManager();
    timer = new QTimer(this);
    timer->setInterval(interval*3600000);
    connect(timer,SIGNAL(timeout()),this,SLOT(Run()));

    cloudPath = rootPath+"/clouds";
    migrationPath = rootPath+"/migration";

}

void Migration::ReadCloud()
{
    QFileInfo info(cloudPath);
    const int size = info.size();
    char buffer[size];
    std::ifstream is;
    is.open(cloudPath.toStdString(),std::ios::binary);
    is.read(buffer,size);
    is.close();
    QJsonDocument clouds_jd = QJsonDocument::fromJson(QByteArray(buffer,size));
    QJsonArray clouds_ja = clouds_jd.array();
    clouds.clear();
    for(int i=0;i<clouds_ja.size();++i)
    {
        QJsonObject cloud_jo = clouds_ja.at(i).toObject();
        CloudInfo cloudInfo;
        cloudInfo.cloudName = cloud_jo["cloud"].toString();
        cloudInfo.account = cloud_jo["account"].toString();
        cloudInfo.addTime = cloud_jo["addtime"].toString();
        cloudInfo.certificate = cloud_jo["key"].toString();
        cloudInfo.defaultBucket = cloud_jo["defaultbucket"].toString();
        clouds.append(cloudInfo);
    }
}

void Migration::ReadMetadata()

{
    //初始化为空
    QByteArray segmentpool_data = "[]";
    QByteArray snapshot_data = "[]";
    //从一个可用的云上获取metadata
    for(int i=0;i<clouds.size();++i)
    {
        CloudClient *client = CreatCloudClient(clouds[i]);
        QNetworkReply *reply = client->GetObject(clouds[i].defaultBucket,"segmentpool");
        if(reply!=NULL)
        {
            QEventLoop eventLoop;
            connect(reply,SIGNAL(finished()),&eventLoop,SLOT(quit()));
            eventLoop.exec();
            if (reply->error() == QNetworkReply::NoError)
            {
                segmentpool_data = reply->readAll();
                segmentpool_metadata = QJsonDocument::fromJson(segmentpool_data);
                reply->deleteLater();
                reply->close();
                delete client;
                break;
            }
        }
        delete client;
    }
    //////////////////////////////////////////////////////////////
    QJsonDocument snapshot_metadata;
    for(int i=0;i<clouds.size();++i)
    {
        CloudClient *client = CreatCloudClient(clouds[i]);
        QNetworkReply *reply = client->GetObject(clouds[i].defaultBucket,"snapshot");
        if(reply!=NULL)
        {
            QEventLoop eventLoop;
            connect(reply,SIGNAL(finished()),&eventLoop,SLOT(quit()));
            eventLoop.exec();
            if (reply->error() == QNetworkReply::NoError)
            {
                snapshot_data = reply->readAll();
                snapshot_metadata = QJsonDocument::fromJson(snapshot_data);
                reply->deleteLater();
                reply->close();
                delete client;
                break;
            }
        }
        delete client;
    }
    //生成buckets
    QJsonArray jarr_buckets = snapshot_metadata.array();
    buckets.clear();
    for(int i=0;i<jarr_buckets.size();++i)
    {
        QJsonObject jo_bucket = jarr_buckets.at(i).toObject();
        Bucket bucket;
        bucket.bucketName =jo_bucket["bucketname"].toString();
        bucket.region =jo_bucket["region"].toString();
        bucket.storageClass = jo_bucket["storageclass"].toString();
        buckets.push_back(bucket);
    }
}

void Migration::WriteMetadata()
{
    CloudClient *client;
    for(int i=0;i<clouds.size();++i)
    {
        client=CreatCloudClient(clouds[i]);
        client->PutObject(clouds[i].defaultBucket,"segmentpool",
                          segmentpool_metadata.toJson(QJsonDocument::Compact));
    }
}

CloudClient * Migration::CreatCloudClient(QString cloudName)
{
    CloudInfo cloud;
    for(int i=0;i<clouds.size();++i)
        if(cloudName==clouds[i].cloudName)
        {
            cloud = clouds[i];
            break;
        }
    return CreatCloudClient(cloud);
}


CloudClient * Migration::CreatCloudClient(CloudInfo cloud)
{
    CloudClient *client = nullptr;
    QStringList keys = cloud.certificate.split('|');
    if(keys.size()==0)
        return client;
    if(cloud.cloudName == "google")
    {
        client = new GoogleClient(manger,keys[0].toStdString().data());
    }
    else
    {
        if(keys.size()<2)
            return nullptr;
        if(cloud.cloudName == "s3")
        {
            S3Client *s3client = new S3Client(manger,keys[0],keys[1]);
            s3client->SetBuckets(buckets);
            client = s3client;
        }
        else if(cloud.cloudName == "azure")
        {
            client = new AzureClient(manger,keys[0],keys[1]);
        }
        else if(cloud.cloudName == "aliyun")
        {
            client = new AliyunClient(manger,keys[0],keys[1]);
        }
        else if(cloud.cloudName == "baiduyun")
        {
            BaiduyunClient *baiduyun = new BaiduyunClient(manger,keys[0],keys[1]);
            baiduyun->SetBuckets(buckets);
            client = baiduyun;
        }
    }
    return client;
}

bool Migration::Fetch(QByteArray &jsonData)
{
    std::ifstream is;
    is.open(migrationPath.toStdString(),std::ios::binary);
    if(!is)
        return false;
    QFileInfo fileInfo(migrationPath);
    int fileSize = fileInfo.size();
    char *buf = new char[fileSize];
    is.read(buf,fileSize);
    is.close();
    jsonData = QByteArray(buf,fileSize);
    delete []buf;
    return true;
}

bool Migration::Pull(const QByteArray &jsondata)
{
    std::ofstream os;
    os.open(migrationPath.toStdString(),std::ios::binary);
    if(!os)
        return false;
    os.write(jsondata.data(),jsondata.size());
    os.close();
    return true;
}

void Migration::Start()
{
    stop = false;
    Run();
    timer->start();
}

void Migration::Stop()
{
    stop = true;
    if(timer->isActive())
        timer->stop();
}

void Migration::Run()
{
    QByteArray jsonData;
    int count =0;
    //读待更新的数据块信息
    if(Fetch(jsonData))
    {
        //读取云的信息（包含密钥）
        ReadCloud();
        //从云上读取metadata
        ReadMetadata();
        //提取待更新的数据块信息（bucketname,blockid,sorceCloud,desCloud）
        QJsonDocument jsonDocument = QJsonDocument::fromJson(jsonData);
        QJsonArray jsonArray = jsonDocument.array();
        for(int i=0;i<jsonArray.size();++i)
        {
            //如果被暂停就终止
            if(stop)
                break;

            QJsonObject jsonObject = jsonArray.at(i).toObject();
            QString bucketName = jsonObject["bucketname"].toString();
            QString sourceCloud = jsonObject["sourcecloud"].toString();
            QString desCloud = jsonObject["descloud"].toString();
            QString blockID = jsonObject["blockid"].toString();
            //迁移数据块
            if(MoveObject(bucketName,blockID,sourceCloud,desCloud))
                ++count;
            else
                break;
        }
        //更新metadata和migration
        Update(jsonData,count);
    }
}

bool Migration::MoveObject(QString bucketName, QString blockID
                           , QString sourceCloud, QString desCloud)
{
    if(sourceCloud==desCloud)
        return true;
    CloudClient *sourceCloudClient = CreatCloudClient(sourceCloud);
    CloudClient *desCloudClient = CreatCloudClient(desCloud);
    if(sourceCloudClient==nullptr||desCloudClient==nullptr)
        return false;
    QEventLoop loop;
    QNetworkReply *reply = sourceCloudClient->GetObject(bucketName,blockID);
    connect(reply,SIGNAL(finished()),&loop,SLOT(quit()));
    loop.exec();
    QByteArray data;
    if(reply->error()==QNetworkReply::NoError)
    {
        data = reply->readAll();
        reply->deleteLater();
        reply->close();
    }
    else
        return false;
    reply = desCloudClient->PutObject(bucketName,blockID,data);
    connect(reply,SIGNAL(finished()),&loop,SLOT(quit()));
    loop.exec();
    if(reply->error()==QNetworkReply::NoError)
    {
        sourceCloudClient->DeleteObject(bucketName,blockID);
        return true;
    }
    else
        return false;
}

void Migration::Update(const QByteArray &jsonData,int count)
{
    QJsonDocument jsonDocument = QJsonDocument::fromJson(jsonData);
    QJsonArray jsonArray = jsonDocument.array();
    //更新metadata
    QJsonArray segmentpool = segmentpool_metadata.array();
    for(int i=0;i<count;++i)
    {
        QJsonObject jsonObject = jsonArray.at(i).toObject();
        QString bucketName = jsonObject["bucketname"].toString();
        QString sourceCloud = jsonObject["sourcecloud"].toString();
        QString desCloud = jsonObject["descloud"].toString();
        QString blockID = jsonObject["blockid"].toString();
        if(sourceCloud==desCloud)
            continue;
        //在metadata中找该项
        for(int j=0;j<segmentpool.size();++j)
        {
            QJsonObject bucket = segmentpool.at(j).toObject();
            if(bucket["bucketname"].toString()!=bucketName)
                continue;
            QJsonArray blocks = bucket["blocks"].toArray();
            for(int k=0;k<blocks.size();++k)
            {
                QJsonObject block = blocks.at(k).toObject();
                if(block["blockid"].toString()!=blockID
                        ||block["cloudname"].toString()!=sourceCloud)
                    continue;
                block.remove("cloudname");
                block.insert("cloudname",desCloud);
                blocks.replace(k,block);
            }
            bucket.remove("blocks");
            bucket.insert("blocks",blocks);
            segmentpool.replace(j,bucket);
        }
    }
    segmentpool_metadata.setArray(segmentpool);
    WriteMetadata();
    //更新migration(删除前面已经更新的项)
    for(int i=count-1;i>=0;--i)
        jsonArray.removeAt(i);
    jsonDocument.setArray(jsonArray);
    Pull(jsonDocument.toJson(QJsonDocument::Compact));
}
