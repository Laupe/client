#include "sharedialog.h"
#include "ui_sharedialog.h"
#include "networkjobs.h"
#include "account.h"
#include "json.h"
#include "folderman.h"
#include <QBuffer>
#include <QMovie>

namespace {
    int SHARETYPE_USER = 0;
    int SHARETYPE_GROUP = 1;
    int SHARETYPE_PUBLIC = 3;

    //int PERM_READ = 1;    sharing always allows reading
    int PERM_UPDATE = 2;
    int PERM_CREATE = 4;
    int PERM_DELETE = 8;
    int PERM_SHARE = 16;
}

namespace OCC {

ShareDialog::ShareDialog(QWidget *parent) :
    QDialog(parent),
    _ui(new Ui::ShareDialog)
{
    setAttribute(Qt::WA_DeleteOnClose);
    _ui->setupUi(this);
    _ui->pushButton_copy->setIcon(QIcon::fromTheme("edit-copy"));
    layout()->setSizeConstraint(QLayout::SetFixedSize);
    QMovie *movie = new QMovie("/home/azelphur/ownCloud-share-tools/loading-icon.gif");
    movie->start();
    _ui->labelShareSpinner->setMovie(movie);
    _ui->labelShareSpinner->hide();

    _ui->labelPasswordSpinner->setMovie(movie);
    _ui->labelPasswordSpinner->hide();

    _ui->labelCalendarSpinner->setMovie(movie);
    _ui->labelCalendarSpinner->hide();
    connect(_ui->checkBox_shareLink, SIGNAL(clicked()), this, SLOT(slotCheckBoxShareLinkClicked()));
    connect(_ui->checkBox_password, SIGNAL(clicked()), this, SLOT(slotCheckBoxPasswordClicked()));
    connect(_ui->lineEdit_password, SIGNAL(returnPressed()), this, SLOT(slotPasswordReturnPressed()));
    connect(_ui->checkBox_expire, SIGNAL(clicked()), this, SLOT(slotCheckBoxExpireClicked()));
    connect(_ui->calendar, SIGNAL(clicked(QDate)), SLOT(slotCalendarClicked(QDate)));

    _ui->labelShareSpinner->hide();
    _ui->lineEdit_shareLink->hide();
    _ui->pushButton_copy->hide();
    _ui->lineEdit_password->hide();
    _ui->checkBox_password->hide();
    _ui->checkBox_expire->hide();
    _ui->calendar->hide();
    _ui->lineEdit_shareGroup->setPlaceholderText(tr("Share with group..."));
    _ui->lineEdit_shareUser->setPlaceholderText(tr("Share with user..."));
    _ui->lineEdit_password->setPlaceholderText(tr("Choose a password for the public link"));

    QStringList headerUser;
    headerUser << "share_id";
    headerUser << tr("User name");
    headerUser << tr("User");
    headerUser << tr("Edit");
    headerUser << tr("Create");
    headerUser << tr("Delete");
    headerUser << tr("Share");
    _ui->treeWidget_shareUser->setHeaderLabels(headerUser);
    _ui->treeWidget_shareUser->setColumnCount(7);
    _ui->treeWidget_shareUser->hideColumn(0);
    connect(_ui->treeWidget_shareUser, SIGNAL(itemChanged(QTreeWidgetItem *, int)), SLOT(slotUserShareWidgetClicked(QTreeWidgetItem *, int)));
    connect(_ui->pushButton_shareUser, SIGNAL(clicked()), SLOT(slotAddUserShareClicked()));
    connect(_ui->lineEdit_shareUser, SIGNAL(returnPressed()), SLOT(slotAddUserShareClicked()));
    connect(_ui->pushButton_user_deleteShare, SIGNAL(clicked()), SLOT(slotDeleteUserShareClicked()));

    QStringList headerGroup;
    headerGroup << "share_id";
    headerGroup << tr("Group");
    headerGroup << tr("Edit");
    headerGroup << tr("Create");
    headerGroup << tr("Delete");
    headerGroup << tr("Share");
    _ui->treeWidget_shareGroup->setHeaderLabels(headerGroup);
    _ui->treeWidget_shareGroup->setColumnCount(6);
    _ui->treeWidget_shareGroup->hideColumn(0);
    connect(_ui->treeWidget_shareGroup, SIGNAL(itemChanged(QTreeWidgetItem *, int)), SLOT(slotGroupShareWidgetClicked(QTreeWidgetItem *, int)));
    connect(_ui->pushButton_shareGroup, SIGNAL(clicked()), SLOT(slotAddGroupShareClicked()));
    connect(_ui->lineEdit_shareGroup, SIGNAL(returnPressed()), SLOT(slotAddGroupShareClicked()));
    connect(_ui->pushButton_group_deleteShare, SIGNAL(clicked()), SLOT(slotDeleteGroupShareClicked()));


}

void ShareDialog::setExpireDate(const QString &date)
{
    _ui->labelCalendarSpinner->show();
    QUrl url = Account::concatUrlPath(AccountManager::instance()->account()->url(), QString("ocs/v1.php/apps/files_sharing/api/v1/shares/").append(QString::number(_public_share_id)));
    QUrl postData;
    QList<QPair<QString, QString> > getParams;
    QList<QPair<QString, QString> > postParams;
    getParams.append(qMakePair(QString::fromLatin1("format"), QString::fromLatin1("json")));
    postParams.append(qMakePair(QString::fromLatin1("expireDate"), date));
    url.setQueryItems(getParams);
    postData.setQueryItems(postParams);
    OcsShareJob *job = new OcsShareJob("PUT", url, postData, AccountManager::instance()->account(), this);
    connect(job, SIGNAL(jobFinished(QString)), this, SLOT(slotExpireSet(QString)));
    job->start();
}

void ShareDialog::slotExpireSet(const QString & /* reply */)
{
    _ui->labelCalendarSpinner->hide();
}

void ShareDialog::slotCalendarClicked(const QDate &date)
{
    ShareDialog::setExpireDate(date.toString("yyyy-MM-dd"));
}

QString ShareDialog::getPath()
{
    return _path;
}

void ShareDialog::setPath(const QString &path)
{
    _path = path;
    ShareDialog::getShares();
}

ShareDialog::~ShareDialog()
{
    delete _ui;
}

void ShareDialog::slotPasswordReturnPressed()
{
    ShareDialog::setPassword(_ui->lineEdit_password->text());
    _ui->lineEdit_password->setPlaceholderText(tr("Password Protected"));
    _ui->lineEdit_password->setText(QString());
}

void ShareDialog::setPassword(QString password)
{
    _ui->labelPasswordSpinner->show();
    QUrl url = Account::concatUrlPath(AccountManager::instance()->account()->url(), QString("ocs/v1.php/apps/files_sharing/api/v1/shares/").append(QString::number(_public_share_id)));
    QUrl postData;
    QList<QPair<QString, QString> > getParams;
    QList<QPair<QString, QString> > postParams;
    getParams.append(qMakePair(QString::fromLatin1("format"), QString::fromLatin1("json")));
    postParams.append(qMakePair(QString::fromLatin1("password"), password));
    url.setQueryItems(getParams);
    postData.setQueryItems(postParams);
    OcsShareJob *job = new OcsShareJob("PUT", url, postData, AccountManager::instance()->account(), this);
    connect(job, SIGNAL(jobFinished(QString)), this, SLOT(slotPasswordSet(QString)));
    job->start();
}

void ShareDialog::slotPasswordSet(const QString & /* reply */)
{
    _ui->labelPasswordSpinner->hide();
}

void ShareDialog::getShares()
{
    this->setWindowTitle(tr("Sharing %1").arg(_path));
    QUrl url = Account::concatUrlPath(AccountManager::instance()->account()->url(), QLatin1String("ocs/v1.php/apps/files_sharing/api/v1/shares"));
    QList<QPair<QString, QString> > params;
    params.append(qMakePair(QString::fromLatin1("format"), QString::fromLatin1("json")));
    params.append(qMakePair(QString::fromLatin1("path"), _path));
    url.setQueryItems(params);
    OcsShareJob *job = new OcsShareJob("GET", url, QUrl(), AccountManager::instance()->account(), this);
    connect(job, SIGNAL(jobFinished(QString)), this, SLOT(slotSharesFetched(QString)));
    job->start();
}

void ShareDialog::slotSharesFetched(const QString &reply)
{
    _ui->treeWidget_shareUser->clear();
    _ui->treeWidget_shareGroup->clear();

    bool success = false;
    QVariantMap json = QtJson::parse(reply, success).toMap();
    ShareDialog::_shares = json.value("ocs").toMap().values("data")[0].toList();
    for(int i = 0; i < ShareDialog::_shares.count(); i++)
    {
        QVariantMap data = ShareDialog::_shares[i].toMap();

        if (data.value("share_type").toInt() == SHARETYPE_USER)
        {
            QStringList columns;

            columns << data.value("id").toString();
            columns << data.value("share_with").toString();
            columns << data.value("share_with_displayname").toString();
            columns << "";
            columns << "";
            columns << "";
            columns << "";

            QTreeWidgetItem *item = new QTreeWidgetItem(columns);

            int perm = data.value("permissions").toInt();

            if (perm & PERM_UPDATE) {
                item->setCheckState(3, Qt::Checked);
            } else {
                item->setCheckState(3, Qt::Unchecked);
            }
            if (perm & PERM_CREATE) {
                item->setCheckState(4, Qt::Checked);
            } else {
                item->setCheckState(4, Qt::Unchecked);
            }
            if (perm & PERM_DELETE) {
                item->setCheckState(5, Qt::Checked);
            } else {
                item->setCheckState(5, Qt::Unchecked);
            }
            if (perm & PERM_SHARE) {
                item->setCheckState(6, Qt::Checked);
            } else {
                item->setCheckState(6, Qt::Unchecked);
            }

            _ui->treeWidget_shareUser->insertTopLevelItem(0, item);
        }

        if (data.value("share_type").toInt() == SHARETYPE_GROUP)
        {
            QStringList columns;

            columns << data.value("id").toString();
            columns << data.value("share_with").toString();
            columns << "";
            columns << "";
            columns << "";
            columns << "";

            QTreeWidgetItem *item = new QTreeWidgetItem(columns);

            int perm = data.value("permissions").toInt();

            if (perm & PERM_UPDATE) {
                item->setCheckState(2, Qt::Checked);
            } else {
                item->setCheckState(2, Qt::Unchecked);
            }
            if (perm & PERM_CREATE) {
                item->setCheckState(3, Qt::Checked);
            } else {
                item->setCheckState(3, Qt::Unchecked);
            }
            if (perm & PERM_DELETE) {
                item->setCheckState(4, Qt::Checked);
            } else {
                item->setCheckState(4, Qt::Unchecked);
            }
            if (perm & PERM_SHARE) {
                item->setCheckState(5, Qt::Checked);
            } else {
                item->setCheckState(5, Qt::Unchecked);
            }
            _ui->treeWidget_shareGroup->insertTopLevelItem(0, item);
        }

        if (data.value("share_type").toInt() == SHARETYPE_PUBLIC)
        {
            _public_share_id = data.value("id").toULongLong();

            _ui->lineEdit_shareLink->show();
            _ui->pushButton_copy->show();
            _ui->checkBox_password->show();
            _ui->checkBox_expire->show();
            _ui->pushButton_copy->show();
            _ui->checkBox_shareLink->setChecked(true);

            if (data.value("share_with").isValid())
            {
                _ui->checkBox_password->setChecked(true);
                _ui->lineEdit_password->setText("********");
                _ui->lineEdit_password->show();
            }

            if (data.value("expiration").isValid())
            {
                _ui->calendar->setSelectedDate(QDate::fromString(data.value("expiration").toString(), "yyyy-MM-dd 00:00:00"));
                _ui->calendar->show();
                _ui->checkBox_expire->setChecked(true);
            }

            const QString url = Account::concatUrlPath(AccountManager::instance()->account()->url(), QString("public.php?service=files&t=%1").arg(data.value("token").toString())).toString();
            _ui->lineEdit_shareLink->setText(url);
        }
    }
}

void ShareDialog::slotDeleteShareFetched(const QString & /* reply */)
{
    _ui->labelShareSpinner->hide();
    _ui->lineEdit_shareLink->hide();
    _ui->pushButton_copy->hide();
    _ui->lineEdit_password->hide();
    _ui->checkBox_password->hide();
    _ui->checkBox_expire->hide();
    _ui->calendar->hide();
}

void ShareDialog::slotCheckBoxShareLinkClicked()
{
    if (_ui->checkBox_shareLink->checkState() == Qt::Checked)
    {
        _ui->labelShareSpinner->show();
        QUrl url = Account::concatUrlPath(AccountManager::instance()->account()->url(), QLatin1String("ocs/v1.php/apps/files_sharing/api/v1/shares"));
        QUrl postData;
        QList<QPair<QString, QString> > getParams;
        QList<QPair<QString, QString> > postParams;
        getParams.append(qMakePair(QString::fromLatin1("format"), QString::fromLatin1("json")));
        postParams.append(qMakePair(QString::fromLatin1("path"), _path));
        postParams.append(qMakePair(QString::fromLatin1("shareType"), QString::number(SHARETYPE_PUBLIC)));
        url.setQueryItems(getParams);
        postData.setQueryItems(postParams);
        OcsShareJob *job = new OcsShareJob("POST", url, postData, AccountManager::instance()->account(), this);
        connect(job, SIGNAL(jobFinished(QString)), this, SLOT(slotCreateShareFetched(QString)));
        job->start();
    }
    else
    {
        _ui->labelShareSpinner->show();
        QUrl url = Account::concatUrlPath(AccountManager::instance()->account()->url(), QString("ocs/v1.php/apps/files_sharing/api/v1/shares/").append(QString::number(_public_share_id)));
        QList<QPair<QString, QString> > getParams;
        getParams.append(qMakePair(QString::fromLatin1("format"), QString::fromLatin1("json")));
        url.setQueryItems(getParams);
        OcsShareJob *job = new OcsShareJob("DELETE", url, QUrl(), AccountManager::instance()->account(), this);
        connect(job, SIGNAL(jobFinished(QString)), this, SLOT(slotDeleteShareFetched(QString)));
        job->start();
    }
}

void ShareDialog::slotCreateShareFetched(const QString &reply)
{
    _ui->labelShareSpinner->hide();
    bool success;
    QVariantMap json = QtJson::parse(reply, success).toMap();
    _public_share_id = json.value("ocs").toMap().values("data")[0].toMap().value("id").toULongLong();
    QString url = json.value("ocs").toMap().values("data")[0].toMap().value("url").toString();
    _ui->lineEdit_shareLink->setText(url);
    _ui->lineEdit_shareLink->show();
    _ui->pushButton_copy->show();
    _ui->checkBox_password->show();
    _ui->checkBox_expire->show();
    _ui->pushButton_copy->show();
}

void ShareDialog::slotCheckBoxPasswordClicked()
{
    if (_ui->checkBox_password->checkState() == Qt::Checked)
    {
        _ui->lineEdit_password->show();
    }
    else
    {
        ShareDialog::setPassword(QString());
        _ui->labelPasswordSpinner->show();
        _ui->lineEdit_password->hide();
    }
}

void ShareDialog::slotCheckBoxExpireClicked()
{
    if (_ui->checkBox_expire->checkState() == Qt::Checked)
    {
        QDate date = QDate::currentDate().addDays(1);
        ShareDialog::setExpireDate(date.toString("dd-MM-yyyy"));
        _ui->calendar->setSelectedDate(date);
        _ui->calendar->show();
    }
    else
    {
        ShareDialog::setExpireDate(QString());
        _ui->calendar->hide();
    }
}

void ShareDialog::slotUserShareWidgetClicked(QTreeWidgetItem *item, const int /* column */)
{

    int id = item->data(0, Qt::DisplayRole).toInt();

    int perm = 1;
    if (item->checkState(3) == Qt::Checked) {
        perm += PERM_UPDATE;
    }
    if (item->checkState(4) == Qt::Checked) {
        perm += PERM_CREATE;
    }
    if (item->checkState(5) == Qt::Checked) {
        perm += PERM_DELETE;
    }
    if (item->checkState(6) == Qt::Checked) {
        perm += PERM_SHARE;
    }

    QUrl url = Account::concatUrlPath(AccountManager::instance()->account()->url(), QString("ocs/v1.php/apps/files_sharing/api/v1/shares/").append(QString::number(id)));
    QUrl postData;
    QList<QPair<QString, QString> > getParams;
    QList<QPair<QString, QString> > postParams;
    getParams.append(qMakePair(QString::fromLatin1("format"), QString::fromLatin1("json")));
    postParams.append(qMakePair(QString::fromLatin1("permissions"), QString::number(perm)));
    url.setQueryItems(getParams);
    postData.setQueryItems(postParams);
    OcsShareJob *job = new OcsShareJob("PUT", url, postData, AccountManager::instance()->account(), this);
    connect(job, SIGNAL(jobFinished(QString)), this, SLOT(slotUpdateUserShare(QString)));
    job->start();
}

void ShareDialog::slotUpdateUserShare(const QString & /*reply*/)
{
}

void ShareDialog::slotAddUserShareClicked()
{
    QUrl url = Account::concatUrlPath(AccountManager::instance()->account()->url(), QLatin1String("ocs/v1.php/apps/files_sharing/api/v1/shares"));
    QUrl postData;
    QList<QPair<QString, QString> > getParams;
    QList<QPair<QString, QString> > postParams;
    getParams.append(qMakePair(QString::fromLatin1("format"), QString::fromLatin1("json")));
    postParams.append(qMakePair(QString::fromLatin1("path"), _path));
    postParams.append(qMakePair(QString::fromLatin1("shareType"), QString::number(SHARETYPE_USER)));
    postParams.append(qMakePair(QString::fromLatin1("shareWith"), _ui->lineEdit_shareUser->text()));

    int perm = 1;
    if (_ui->checkBox_user_edit->checkState() == Qt::Checked) {
        perm += PERM_UPDATE;
    }
    if (_ui->checkBox_user_create->checkState() == Qt::Checked) {
        perm += PERM_CREATE;
    }
    if (_ui->checkBox_user_delete->checkState() == Qt::Checked) {
        perm += PERM_DELETE;
    }
    if (_ui->checkBox_user_reshare->checkState() == Qt::Checked) {
        perm += PERM_SHARE;
    }

    postParams.append(qMakePair(QString("permissions"), QString::number(perm)));
    url.setQueryItems(getParams);
    postData.setQueryItems(postParams);
    OcsShareJob *job = new OcsShareJob("POST", url, postData, AccountManager::instance()->account(), this);
    connect(job, SIGNAL(jobFinished(QString)), this, SLOT(slotAddUserShareReply(QString)));
    job->start();
}

void ShareDialog::slotAddUserShareReply(const QString &/*reply*/)
{
    getShares();
}

void ShareDialog::slotDeleteUserShareClicked()
{
    auto items = _ui->treeWidget_shareUser->selectedItems();
    if (items.empty()) {
        return;
    }

    auto item = items.at(0);
    int id = item->data(0, Qt::DisplayRole).toInt();

     QUrl url = Account::concatUrlPath(AccountManager::instance()->account()->url(), QString("ocs/v1.php/apps/files_sharing/api/v1/shares/%1").arg(id));
    QUrl postData;
    QList<QPair<QString, QString> > getParams;
    QList<QPair<QString, QString> > postParams;
    getParams.append(qMakePair(QString::fromLatin1("format"), QString::fromLatin1("json")));
    url.setQueryItems(getParams);
    postData.setQueryItems(postParams);
    OcsShareJob *job = new OcsShareJob("DELETE", url, postData, AccountManager::instance()->account(), this);
    connect(job, SIGNAL(jobFinished(QString)), this, SLOT(slotDeleteUserShareReply(QString)));
    job->start();

}

void ShareDialog::slotDeleteUserShareReply(const QString &/*reply*/)
{
    getShares();
}


void ShareDialog::slotGroupShareWidgetClicked(QTreeWidgetItem *item, const int /*column*/)
{

    int id = item->data(0, Qt::DisplayRole).toInt();

    int perm = 1;
    if (item->checkState(2) == Qt::Checked) {
        perm += PERM_UPDATE;
    }
    if (item->checkState(3) == Qt::Checked) {
        perm += PERM_CREATE;
    }
    if (item->checkState(4) == Qt::Checked) {
        perm += PERM_DELETE;
    }
    if (item->checkState(5) == Qt::Checked) {
        perm += PERM_SHARE;
    }

    QUrl url = Account::concatUrlPath(AccountManager::instance()->account()->url(), QString("ocs/v1.php/apps/files_sharing/api/v1/shares/").append(QString::number(id)));
    QUrl postData;
    QList<QPair<QString, QString> > getParams;
    QList<QPair<QString, QString> > postParams;
    getParams.append(qMakePair(QString::fromLatin1("format"), QString::fromLatin1("json")));
    postParams.append(qMakePair(QString::fromLatin1("permissions"), QString::number(perm)));
    url.setQueryItems(getParams);
    postData.setQueryItems(postParams);
    OcsShareJob *job = new OcsShareJob("PUT", url, postData, AccountManager::instance()->account(), this);
    connect(job, SIGNAL(jobFinished(QString)), this, SLOT(slotUpdateGroupShare(QString)));
    job->start();
}

void ShareDialog::slotUpdateGroupShare(const QString &/*reply*/)
{
    getShares();
}

void ShareDialog::slotAddGroupShareClicked()
{
    QUrl url = Account::concatUrlPath(AccountManager::instance()->account()->url(), QLatin1String("ocs/v1.php/apps/files_sharing/api/v1/shares"));
    QUrl postData;
    QList<QPair<QString, QString> > getParams;
    QList<QPair<QString, QString> > postParams;
    getParams.append(qMakePair(QString::fromLatin1("format"), QString::fromLatin1("json")));
    postParams.append(qMakePair(QString::fromLatin1("path"), _path));
    postParams.append(qMakePair(QString::fromLatin1("shareType"), QString::number(SHARETYPE_GROUP)));
    postParams.append(qMakePair(QString::fromLatin1("shareWith"), _ui->lineEdit_shareGroup->text()));

    int perm = 1;
    if (_ui->checkBox_group_edit->checkState() == Qt::Checked) {
        perm += PERM_UPDATE;
    }
    if (_ui->checkBox_group_create->checkState() == Qt::Checked) {
        perm += PERM_CREATE;
    }
    if (_ui->checkBox_group_delete->checkState() == Qt::Checked) {
        perm += PERM_DELETE;
    }
    if (_ui->checkBox_group_reshare->checkState() == Qt::Checked) {
        perm += PERM_SHARE;
    }

    postParams.append(qMakePair(QString("permissions"), QString::number(perm)));
    url.setQueryItems(getParams);
    postData.setQueryItems(postParams);
    OcsShareJob *job = new OcsShareJob("POST", url, postData, AccountManager::instance()->account(), this);
    connect(job, SIGNAL(jobFinished(QString)), this, SLOT(slotAddGroupShareReply(QString)));
    job->start();
}

void ShareDialog::slotAddGroupShareReply(const QString &reply)
{
    qDebug() << Q_FUNC_INFO << reply;
    getShares();
}

void ShareDialog::slotDeleteGroupShareClicked()
{
    auto items = _ui->treeWidget_shareGroup->selectedItems();
    if (items.empty()) {
        return;
    }

    auto item = items.at(0);
    int id = item->data(0, Qt::DisplayRole).toInt();

     QUrl url = Account::concatUrlPath(AccountManager::instance()->account()->url(), QString("ocs/v1.php/apps/files_sharing/api/v1/shares/%1").arg(id));
    QUrl postData;
    QList<QPair<QString, QString> > getParams;
    QList<QPair<QString, QString> > postParams;
    getParams.append(qMakePair(QString::fromLatin1("format"), QString::fromLatin1("json")));
    url.setQueryItems(getParams);
    postData.setQueryItems(postParams);
    OcsShareJob *job = new OcsShareJob("DELETE", url, postData, AccountManager::instance()->account(), this);
    connect(job, SIGNAL(jobFinished(QString)), this, SLOT(slotDeleteGroupShareReply(QString)));
    job->start();

}

void ShareDialog::slotDeleteGroupShareReply(const QString &/*reply*/)
{
    getShares();
}




OcsShareJob::OcsShareJob(const QByteArray &verb, const QUrl &url, const QUrl &postData, AccountPtr account, QObject* parent)
: AbstractNetworkJob(account, "", parent),
  _verb(verb),
  _url(url),
  _postData(postData)

{
    setIgnoreCredentialFailure(true);
}

void OcsShareJob::start()
{
    QNetworkRequest req;
    req.setRawHeader("OCS-APIREQUEST", "true");
    req.setRawHeader("Content-Type", "application/x-www-form-urlencoded");
    QBuffer *buffer = new QBuffer;

    QStringList tmp;
    Q_FOREACH(auto tmp2, _postData.queryItems()) {
        tmp.append(tmp2.first + "=" + tmp2.second);
    }
    buffer->setData(tmp.join("&").toAscii());

    setReply(davRequest(_verb, _url, req, buffer));
    setupConnections(reply());
    buffer->setParent(reply());
    AbstractNetworkJob::start();
}

bool OcsShareJob::finished()
{
    emit jobFinished(reply()->readAll());
    return true;
}



}
