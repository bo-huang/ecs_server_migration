#ifndef MIGRATION_H
#define MIGRATION_H

#include<QByteArray>
#include<QJsonDocument>
#include<CLASS/cloudinfo.h>
#include<CloudSDK/cloudclient.h>
#include<vector>
#include<CLASS/bucket.h>
#include<QObject>
#include<QNetworkAccessManager>
#include<QTimer>

class Migration : public QObject
{
    Q_OBJECT
public:
    Migration(QString directory,int interval);
    void Start();
    void Stop();
private:
     void ReadCloud();
     void ReadMetadata();
     void WriteMetadata();
     //取出待更新的数据
     bool Fetch(QByteArray &jsonData);
     //写回还未完成的更新数据
     bool Pull(const QByteArray &jsondata);
     CloudClient * CreatCloudClient(QString cloudName);
     CloudClient * CreatCloudClient(CloudInfo cloud);
     //移动数据块
     bool MoveObject(QString bucketName,QString objectName
                     ,QString sourceCloud,QString desCloud);
     //更新元数据到云以及更新migration文件（有可能迁移了一部分就被终止了）
     void Update(const QByteArray &jsonData,int count);
private slots:
     //开始迁移数据
     void Run();
private:
    QList<CloudInfo> clouds;
    QJsonDocument segmentpool_metadata;
    std::vector<Bucket>buckets;
    QNetworkAccessManager *manger;
    QString rootPath;
    QString cloudPath;
    QString migrationPath;
    int interval;//单位小时
    QTimer *timer;
    bool stop;
};

#endif // MIGRATION_H
