#pragma once

#include <QtWidgets/QtWidgets>

class CSV
{
public:
    using Row = QStringList;

    enum WriteFlags : int
    {
        None = 0x0,
        NoComments = 0x1,
        EnsureSameColumnSize = 0x2,

        Default = None
    };

    CSV() = default;
    ~CSV() = default;

    QList<Row>& rows() { return m_rows; }
    size_t rowCount() const { return static_cast<size_t>(m_rows.size()); }
    void addRow(const Row& row) { m_rows.push_back(row); }

    size_t maxColumnCount() const;

    void readFile(const QString& filePath, char delimiter = ',');
    void writeFile(const QString& filePath, char delimiter = ',', WriteFlags flags = Default);

private:
    QList<Row> m_rows;
};