#include "seriesmodel.h"
#include <QJsonArray>

SeriesModel::SeriesModel(QObject *parent, Komga_api* api) :
    QAbstractListModel{parent}, m_api{api}
{
    connect(m_api, &Komga_api::seriesDataReady,
            this, &SeriesModel::apiDataReceived);
}
int SeriesModel::rowCount(const QModelIndex &parent) const {
    Q_ASSERT(checkIndex(parent));
    if (parent.isValid()) {
        return 0;
    }
    return m_series.count();
}
QVariant SeriesModel::data(const QModelIndex &index, int role) const {
    Q_ASSERT(checkIndex(index, QAbstractItemModel::CheckIndexOption::IndexIsValid));
    if (! index.isValid()) {
        qDebug() << "invalid indx " << index.row();
        return QVariant();
    }
    if (index.row() < 0 || index.row() >= m_series.count())
        return QVariant();

    const Series* series = m_series[index.row()];
    if (series) {
        if (role == NameRole)
            return series->name();
        else if (role == IdRole)
            return series->id();
        else if (role == LibraryIdRole)
            return series->libraryId();
        else if (role == BookCountRole)
            return series->booksCount();
        else if (role == UrlRole)
            return series->url();
        else if (role == MetadataStatusRole)
            return series->metadataStatus();
    }
    return QVariant();
}
QHash<int, QByteArray> SeriesModel::roleNames() const {
    QHash<int, QByteArray> roles;
        roles[IdRole] = "seriesId";
        roles[NameRole] = "seriesName";
        roles[LibraryIdRole] = "seriesLibraryId";
        roles[BookCountRole] = "seriesBookCount";
        roles[UrlRole] = "seriesUrl";
        roles[MetadataStatusRole] = "seriesMetadataStatus";
        return roles;
}
void SeriesModel::loadSeries(int library) {
        m_api->getSeries(library);
        resetSeries();
}
void SeriesModel::resetSeries() {
    qDebug() << "reset series ";
    emit beginResetModel();
    qDeleteAll(m_series);
    m_series.clear();
    emit endResetModel();
}

void SeriesModel::apiDataReceived(QJsonObject page) {
    int pageNum = page["number"].toInt();
    int totPages = page["totalPages"].toInt();
    int nbElems = page["numberOfElements"].toInt();
    qDebug() << "page : " << pageNum;
    qDebug() << "page number : " << totPages;
    if (nbElems > 0) {
        if (pageNum > 0) {
            emit beginInsertRows(QModelIndex(), m_series.size(), m_series.size() + nbElems - 1);
            QJsonArray content = page["content"].toArray();
            foreach (const QJsonValue &value, content) {
                Series* s = new Series(this);
                QJsonObject jsob = value.toObject();
                qDebug() << jsob["id"].toInt();
                qDebug() << jsob["name"].toString();
                s->setId(jsob["id"].toInt());
                QString n = jsob["name"].toString();
                s->setName(n);
                s->setBooksCount(jsob["booksCount"].toInt());
                s->setLibraryId(jsob["libraryId"].toInt());
                QString u = jsob["url"].toString();
                s->setUrl(u);
                QJsonObject metadata = jsob["metadata"].toObject();
                s->setMetadataTitle(metadata["title"].toString());
                s->setMetadataStatus(metadata["status"].toString());
                m_series.append(std::move(s));
            }
            emit endInsertRows();
        }
        else {
            QList<Series*> series{};
            QJsonArray content = page["content"].toArray();
            foreach (const QJsonValue &value, content) {
                Series* s = new Series(this);
                QJsonObject jsob = value.toObject();
                qDebug() << jsob["id"].toInt();
                qDebug() << jsob["name"].toString();
                s->setId(jsob["id"].toInt());
                QString n = jsob["name"].toString();
                s->setName(n);
                s->setBooksCount(jsob["booksCount"].toInt());
                s->setLibraryId(jsob["libraryId"].toInt());
                QString u = jsob["url"].toString();
                s->setUrl(u);
                QJsonObject metadata = jsob["metadata"].toObject();
                s->setMetadataTitle(metadata["title"].toString());
                s->setMetadataStatus(metadata["status"].toString());
                series.append(std::move(s));
            }

            emit layoutAboutToBeChanged();
//            emit beginResetModel();
//            qDeleteAll(m_series);
//            m_series.clear();
//            emit beginInsertRows(QModelIndex(), 0, series.size() - 1);
            m_series = series;
//            m_series.append(series);
            changePersistentIndex(index(0), QModelIndex());
            emit layoutChanged();
//            emit refresh();
//            emit endInsertRows();
//            emit endResetModel();
        }
        m_currentPageNumber = pageNum;
        m_totalPageNumber = totPages;
    }
}
QByteArray SeriesModel::getThumbnail(int id) {
    QByteArray a = m_api->getThumbnail(id, Komga_api::ThumbnailType::SeriesThumbnail);
    return a;
}
void SeriesModel::nextSeriesPage(int libraryId) {
    qDebug() << "curr p :" << m_currentPageNumber << " total p : " << m_totalPageNumber;
    if (m_currentPageNumber + 1 < m_totalPageNumber) {
        m_api->getSeries(libraryId, m_currentPageNumber + 1);
    }
}
Series* SeriesModel::find(int libraryId) {
    auto itr = std::find_if(m_series.begin(), m_series.end(), [libraryId](Series* series) { return series->libraryId() == libraryId; });
    if(itr != m_series.end()) {
        qDebug() << "find " << (*itr)->name();
        return *itr;
    }
    return nullptr;
}
