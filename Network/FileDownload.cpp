
#include "FileDownload.hpp"
#include <QCoreApplication>
#include <QUrl>
#include <QNetworkRequest>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QFileInfo>
#include <QDir>
#include <QTemporaryFile>
#include "qt_helpers.hpp"
#include "Logger.hpp"

FileDownload::FileDownload() : QObject(nullptr)
{
  redirect_count_ = 0;
  url_valid_ = false;
}

FileDownload::~FileDownload()
{
}

void FileDownload::errorOccurred(QNetworkReply::NetworkError code)
{
  LOG_INFO(QString{"FileDownload [%1]: errorOccurred %2 -> %3"}.arg(user_agent_).arg(code).arg(reply_->errorString()));
  Q_EMIT error (reply_->errorString ());
  delete tmpfile_;
}

void FileDownload::configure(QNetworkAccessManager *network_manager, const QString &source_url, const QString &destination_path, const QString &user_agent)
{
  manager_ = network_manager;
  source_url_ = source_url;
  destination_filename_ = destination_path;
  user_agent_ = user_agent;
}

void FileDownload::store()
{
  if (tmpfile_->isOpen())
    tmpfile_->write (reply_->read (reply_->bytesAvailable ()));
  else
    LOG_INFO(QString{ "FileDownload [%1]: tmpfile is not open"}.arg(user_agent_));
}

void FileDownload::replyComplete()
{
  QFileInfo destination_file(destination_filename_);
  QString const tmpfile_path = destination_file.absolutePath();
  QDir tmpdir_(tmpfile_path);

  LOG_DEBUG(QString{ "FileDownload [%1]: replyComplete"}.arg(user_agent_));
  if (!reply_)
  {
    Q_EMIT load_finished ();
    return;           // we probably deleted it in an earlier call
  }

  QUrl redirect_url {reply_->attribute (QNetworkRequest::RedirectionTargetAttribute).toUrl ()};

  if (reply_->error () == QNetworkReply::NoError && !redirect_url.isEmpty ())
  {
    if ("https" == redirect_url.scheme () && !QSslSocket::supportsSsl ())
    {
      Q_EMIT download_error (tr ("Network Error - SSL/TLS support not installed, cannot fetch:\n\'%1\'")
                                              .arg (redirect_url.toDisplayString ()));
      url_valid_ = false; // reset
      Q_EMIT load_finished ();
    }
    else if (++redirect_count_ < 10) // maintain sanity
    {
      // follow redirect
      download (reply_->url ().resolved (redirect_url));
    }
    else
    {
      Q_EMIT download_error (tr ("Network Error - Too many redirects:\n\'%1\'")
                                              .arg (redirect_url.toDisplayString ()));
      url_valid_ = false; // reset
      Q_EMIT load_finished ();
    }
  }
  else if (reply_->error () != QNetworkReply::NoError)
  {
    tmpfile_->close();
    delete tmpfile_;
    url_valid_ = false;     // reset
    // report errors that are not due to abort
    if (QNetworkReply::OperationCanceledError != reply_->error ())
    {
      Q_EMIT download_error (tr ("Network Error:\n%1")
                                              .arg (reply_->errorString ()));
    }
    Q_EMIT load_finished ();
  }
  else
  {
      if (!url_valid_)
      {
        // now get the body content
        url_valid_ = true;
        download (reply_->url ().resolved (redirect_url));
      }
      else // the body has completed. Save it.
      {
        url_valid_ = false; // reset
        // load the database asynchronously
        // future_load_ = std::async (std::launch::async, &LotWUsers::impl::load_dictionary, this, csv_file_.fileName ());
        LOG_INFO(QString{ "FileDownload [%1]: complete. tempfile path is %2"}.arg(user_agent_).arg(tmpfile_->fileName()));
        // move the file to the destination
        tmpdir_.remove(destination_filename_+".old"); // get rid of previous version
        tmpdir_.rename(destination_filename_, destination_filename_+".old");
        tmpdir_.rename(tmpfile_->fileName(), destination_filename_);
        LOG_INFO(QString{ "FileDownload [%1]: moved tempfile %2 to %3"}.arg(user_agent_).arg(tmpfile_->fileName()).arg(destination_filename_));
        tmpfile_->close();
        delete tmpfile_;
        emit complete(destination_filename_);
      }
  }

  if (reply_ && reply_->isFinished ())
  {
    reply_->deleteLater ();
  }

}

void FileDownload::downloadComplete(QNetworkReply *data)
{
  // make a temp file in the same place as the file we're downloading. Needs to be on the same
  // filesystem as where we eventually want to 'mv' it.

  QUrl r = request_.url();
  LOG_INFO(QString{"FileDownload [%1]: finished %2 of %3 -> %4 (%5)"}.arg(user_agent_).arg(data->operation()).arg(source_url_).arg(destination_filename_).arg(r.url()));

#ifdef DEBUG_FILEDOWNLOAD
  LOG_INFO("Request Headers:");
  Q_FOREACH (const QByteArray& hdr, request_.rawHeaderList()) {
      LOG_INFO(QString{ "%1 -> %2"}.arg(QString(hdr)).arg(QString(request_.rawHeader(hdr))));
  }

  LOG_INFO("Response Headers:");
  Q_FOREACH (const QByteArray& hdr, reply_->rawHeaderList()) {
      LOG_INFO(QString{ "%1 -> %2"}.arg(QString(hdr)).arg(QString(reply_->rawHeader(hdr))));
  }
#endif
  data->deleteLater();
}

void FileDownload::start_download()
{
  url_valid_ = false;
  download(QUrl(source_url_));
}

void FileDownload::download(QUrl qurl)
{
  request_.setUrl(qurl);

#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
  if (QNetworkAccessManager::Accessible != manager_->networkAccessible ())
      {
        // try and recover network access for QNAM
        manager_->setNetworkAccessible (QNetworkAccessManager::Accessible);
      }
#endif

  LOG_INFO(QString{"FileDownload [%1]: Starting download of %2 to %3"}.arg(user_agent_).arg(source_url_).arg(destination_filename_));

  request_.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
  request_.setRawHeader("Accept", "*/*");
  request_.setRawHeader ("User-Agent", user_agent_.toLocal8Bit());  // Must have a UA for some sites, like country-files

  if (!url_valid_)
  {
    reply_ = manager_->head(request_);
  }
  else
  {
    reply_ = manager_->get (request_);
  }

  QObject::connect(manager_, &QNetworkAccessManager::finished, this, &FileDownload::downloadComplete, Qt::UniqueConnection);
  QObject::connect(reply_, &QNetworkReply::downloadProgress, this, &FileDownload::downloadProgress, Qt::UniqueConnection);
  QObject::connect(reply_, &QNetworkReply::finished, this,&FileDownload::replyComplete, Qt::UniqueConnection);

  QObject::connect(reply_, &QNetworkReply::errorOccurred,this,&FileDownload::errorOccurred, Qt::UniqueConnection);
  QObject::connect (reply_, &QNetworkReply::readyRead, this, &FileDownload::store, Qt::UniqueConnection);

  QFileInfo destination_file(destination_filename_);
  QString const tmpfile_base = destination_file.fileName();
  QString const tmpfile_path = destination_file.absolutePath();
  tmpfile_ = new QTemporaryFile(tmpfile_path+QDir::separator()+tmpfile_base+".XXXXXX");
  if (!tmpfile_->open())
  {
    LOG_INFO(QString{"FileDownload [%1]: Unable to open the temporary file based on %2"}.arg(user_agent_).arg(tmpfile_path));
    return;
  }
}

void FileDownload::downloadProgress(qint64 received, qint64 total)
{
  LOG_DEBUG(QString{"FileDownload: [%1] Progress %2 from %3, total %4, so far %5"}.arg(user_agent_).arg(destination_filename_).arg(source_url_).arg(total).arg(received));
  Q_EMIT progress(QString{"%4 bytes downloaded"}.arg(received));
}

void FileDownload::abort ()
{
  if (reply_ && reply_->isRunning ())
  {
    reply_->abort ();
  }
}