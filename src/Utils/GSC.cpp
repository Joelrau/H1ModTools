#include "GSC.h"

#include "QTUtils.h"

struct FunctionMapping
{
    QString oldName;
    QString newName;
    int minArgs = -1;
    std::function<QString(const QStringList&)> transformer;
};

struct InsertionRule
{
    QString afterCall;
    QString insertCode;
};

using RemovalMap = QHash<QString, QSet<QString>>;
using InsertionMap = QHash<QString, QVector<InsertionRule>>;
using MappingMap = QHash<QString, FunctionMapping>;


//-----------------------------------------------------
// Parse full function body using brace matching
//-----------------------------------------------------
static int FindFunctionEnd(const QString& content, int bodyStart)
{
    int depth = 1;

    for (int i = bodyStart; i < content.size(); i++)
    {
        switch (content[i].unicode())
        {
        case '{': depth++; break;
        case '}': depth--; break;
        }

        if (depth == 0)
            return i + 1;
    }

    return content.size();
}


//-----------------------------------------------------
// Process individual function block
//-----------------------------------------------------
static QString ProcessFunctionBlock(
    const QString& functionName,
    QStringView block,
    const RemovalMap& removalRules,
    const InsertionMap& insertionRules)
{
    QStringList lines = block.toString().split('\n');
    QStringList output;

    const auto removals = removalRules.value(functionName);
    const auto insertions = insertionRules.value(functionName);

    int braceLine = -1;

    for (int i = 0; i < lines.size(); i++)
    {
        output << lines[i];

        if (braceLine == -1 && lines[i].contains('{'))
        {
            braceLine = i;

            // insert immediately after opening brace
            for (const auto& rule : insertions)
            {
                if (rule.afterCall.isEmpty())
                    output << "    " + rule.insertCode;
            }

            continue;
        }

        if (braceLine == -1)
            continue;

        QString trimmed = lines[i].trimmed();

        //-------------------------------------------------
        // Remove function calls
        //-------------------------------------------------
        bool removeLine = false;

        for (const auto& removeFunc : removals)
        {
            if (trimmed.startsWith(removeFunc + "("))
            {
                output.removeLast();
                removeLine = true;
                break;
            }
        }

        if (removeLine)
            continue;

        //-------------------------------------------------
        // Insert after function calls
        //-------------------------------------------------
        for (const auto& rule : insertions)
        {
            if (!rule.afterCall.isEmpty() &&
                trimmed.startsWith(rule.afterCall + "("))
            {
                output << "    " + rule.insertCode;
            }
        }
    }

    return output.join('\n');
}


//-----------------------------------------------------
// Transform function calls globally
//-----------------------------------------------------
static QString TransformFunctionCalls(
    const QString& input,
    const MappingMap& mappings)
{
    static const QRegularExpression callRegex(
        R"((self\s+)?([A-Za-z0-9_:\\]+)\s*\(([^)]*)\))"
    );

    QString output;
    output.reserve(input.size());

    int lastPos = 0;

    auto matches = callRegex.globalMatch(input);

    while (matches.hasNext())
    {
        auto match = matches.next();

        output += QStringView(input).mid(lastPos,
            match.capturedStart() - lastPos);

        const QString selfPrefix = match.captured(1);
        const QString functionName = match.captured(2);

        if (!mappings.contains(functionName))
        {
            output += match.captured(0);
            lastPos = match.capturedEnd();
            continue;
        }

        QStringList args =
            match.captured(3).split(',', Qt::SkipEmptyParts);

        for (auto& arg : args)
            arg = arg.trimmed();

        const auto& mapping = mappings[functionName];

        if (mapping.minArgs != -1 &&
            args.size() < mapping.minArgs)
        {
            output += match.captured(0);
            lastPos = match.capturedEnd();
            continue;
        }

        QString replacement;

        if (mapping.transformer)
        {
            replacement = mapping.transformer(args);
        }
        else
        {
            replacement = QString("%1%2(%3)")
                .arg(selfPrefix)
                .arg(mapping.newName)
                .arg(args.join(", "));
        }

        output += replacement;
        lastPos = match.capturedEnd();
    }

    output += QStringView(input).mid(lastPos);

    return output;
}


//-----------------------------------------------------
// Main conversion
//-----------------------------------------------------
void ConvertGSCFile(
    const QString& gscPath,
    const GSC_Convert_Settings& settings)
{
    QFile file(gscPath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "Failed reading:" << gscPath;
        return;
    }

    QString content = QTextStream(&file).readAll();
    file.close();

    //-------------------------------------------------
    // Build mappings
    //-------------------------------------------------
    MappingMap mappings =
    {
        {
            "wait",
            {
                "wait",
                "wait",
                1,
                [](const QStringList& args)
                {
                    return args.isEmpty()
                        ? "wait();"
                        : QString("wait(%1);").arg(args[0]);
                }
            }
        },

        {
            "maps\\mp\\_utility::createOneshotEffect",
            {
                "maps\\mp\\_utility::createOneshotEffect",
                "common_scripts\\utility::createOneshotEffect"
            }
        },

        {
            "maps\\mp\\_utility::createLoopEffect",
            {
                "maps\\mp\\_utility::createLoopEffect",
                "common_scripts\\utility::createLoopEffect"
            }
        }
    };

    //-------------------------------------------------
    // Removal rules
    //-------------------------------------------------
    RemovalMap removals =
    {
        { "main", { "setExpFog" } }
    };

    //-------------------------------------------------
    // Insertion rules
    //-------------------------------------------------
    InsertionMap insertions;

    const QString loadName = settings.isMpMap
        ? "maps\\mp\\_load::main"
        : "maps\\_load::main";

    if (content.contains(loadName))
    {
        auto& mainRules = insertions["main"];

        if (!content.contains("_art::main()"))
        {
            mainRules.push_back({
                "",
                QString("maps\\createart\\%1_art::main();")
                    .arg(QFileInfo(gscPath).baseName())
                });
        }

        if (settings.hasDestructibles)
            mainRules.push_back({
                loadName,
                "thread common_scripts\\_destructible::init();"
                });

        if (settings.hasPipes)
            mainRules.push_back({
                loadName,
                "thread common_scripts\\_pipes::init();"
                });

        if (settings.hasAnimatedModels)
            mainRules.push_back({
                loadName,
                settings.isMpMap
                    ? "thread maps\\mp\\_animatedmodels::main();"
                    : "thread maps\\_animatedmodels::main();"
                });
    }

    //-------------------------------------------------
    // Parse functions
    //-------------------------------------------------
    static const QRegularExpression functionRegex(
        R"(^\s*(\w+)\s*\([^)]*\)\s*\n?\s*\{)",
        QRegularExpression::MultilineOption
    );

    QString output;
    output.reserve(content.size());

    int offset = 0;
    int searchPos = 0;

    while (true)
    {
        auto match = functionRegex.match(content, searchPos);

        if (!match.hasMatch())
            break;

        const QString functionName = match.captured(1);
        const int start = match.capturedStart();
        const int bodyStart = match.capturedEnd();

        output += QStringView(content).mid(offset, start - offset);

        int end = FindFunctionEnd(content, bodyStart);

        QStringView functionBlock(content.data() + start, end - start);

        output += ProcessFunctionBlock(
            functionName,
            functionBlock,
            removals,
            insertions
        );

        offset = end;
        searchPos = end;
    }

    output += QStringView(content).mid(offset);

    //-------------------------------------------------
    // Final transform pass
    //-------------------------------------------------
    output = TransformFunctionCalls(output, mappings);

    //-------------------------------------------------
    // Write result
    //-------------------------------------------------
    if (!file.open(
        QIODevice::WriteOnly |
        QIODevice::Text |
        QIODevice::Truncate))
    {
        qDebug() << "Failed writing:" << gscPath;
        return;
    }

    QTextStream(&file) << output;
    file.close();

    qDebug() << "Converted:" << gscPath;
}

QStringList findAllGSCFiles(const QString& root)
{
	QStringList result;
	QDirIterator it(root, QStringList() << "*.gsc", QDir::Files, QDirIterator::Subdirectories);
	while (it.hasNext())
		result << it.next();
	return result;
}

void ConvertGSCFiles(const QString& destinationPath, GSC_Convert_Settings settings)
{
	// For each gsc file in destinationPath, convert the GSC file.
	QDir dir(destinationPath);
	if (!dir.exists()) {
		qDebug() << "Directory does not exist:" << destinationPath;
		return;
	}
	QStringList gscFiles = findAllGSCFiles(destinationPath);
	for (const QString& path : gscFiles) {
		qDebug() << "[ConvertGSCFiles] Converting GSC file:" << path;
		ConvertGSCFile(path, settings);
	}
	qDebug() << "All GSC files converted in:" << destinationPath;
}