#include "CSV.h"

size_t CSV::maxColumnCount() const
{
    size_t maxColumns = 0;
    for (const auto& row : m_rows)
    {
        maxColumns = std::max(maxColumns, static_cast<size_t>(row.size()));
    }
    return maxColumns;
}

void CSV::readFile(const QString& filePath, char delimiter)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qWarning("CSV::readFile - Failed to open file: %s", qPrintable(filePath));
        return;
    }

    QTextStream stream(&file);
    m_rows.clear();

    while (!stream.atEnd())
    {
        QString line = stream.readLine().trimmed();
        if (line.isEmpty())
            continue;

        Row row = line.split(delimiter, Qt::KeepEmptyParts);
        m_rows.push_back(row);
    }

    file.close();
}

void CSV::writeFile(const QString& filePath, char delimiter, WriteFlags flags)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qWarning("CSV::writeFile - Failed to open file for writing: %s", qPrintable(filePath));
        return;
    }

    QTextStream stream(&file);
    const size_t maxColumns = maxColumnCount();

    for (const auto& row : m_rows)
    {
        if ((flags & NoComments) && !row.isEmpty())
        {
            if (row.first().startsWith("//") || row.first().startsWith("#"))
                continue;
        }

        Row outputRow = row;

        if ((flags & EnsureSameColumnSize) && static_cast<size_t>(outputRow.size()) < maxColumns)
        {
            outputRow.resize(maxColumns);
        }

        stream << outputRow.join(delimiter) << '\n';
    }

    file.close();
}