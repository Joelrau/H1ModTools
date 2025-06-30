#include "GSC.h"

void CopyGSCFiles(const QString& destinationPath)
{
	QString sourcePath = "static/rawfiles/";
	QtUtils::copyDirectory(sourcePath, destinationPath);
}

struct FunctionMapping {
	QString oldName;
	QString newName;
	int minArgs = -1;
	std::function<QString(const QStringList&)> transformer;
};

struct InsertionRule {
	QString targetFunction;
	QString afterCall;
	QString insertCode;
};

struct RemovalRule {
	QString functionName;
	QStringList functionsToRemove;
};

QString processFunctionBlock(const QString& functionName, const QString& fullFunctionBlock,
    const QHash<QString, QVector<QString>>& removeRules,
    const QVector<InsertionRule>& insertionRules)
{
    // fullFunctionBlock contains the full function including signature and braces,
    // so we need to split out the function header line and the body lines

    QStringList lines = fullFunctionBlock.split('\n');
    QStringList result;

    // Find line with '{' to identify where function body starts
    int braceLineIndex = -1;
    int bracePosInLine = -1;
    for (int i = 0; i < lines.size(); ++i) {
        int pos = lines[i].indexOf('{');
        if (pos != -1) {
            braceLineIndex = i;
            bracePosInLine = pos;
            break;
        }
    }

    if (braceLineIndex == -1) {
        // Malformed function block: no opening brace found, return as is
        return fullFunctionBlock;
    }

    // Copy function signature lines (up to and including brace line)
    for (int i = 0; i <= braceLineIndex; ++i) {
        result << lines[i];
    }

    bool insertedAtStart = false;

    // Process function body lines, start after brace line
    for (int i = braceLineIndex + 1; i < lines.size(); ++i) {
        QString line = lines[i];
        QString trimmed = line.trimmed();

        // Removal logic: skip lines that contain calls to functions to remove
        if (removeRules.contains(functionName)) {
            bool shouldRemove = false;
            for (const QString& toRemove : removeRules[functionName]) {
                // Regex: ^funcName\s*\([^;]*\)\s*;?$
                QRegularExpression pattern("^" + QRegularExpression::escape(toRemove) + R"(\s*\([^;]*\)\s*;?\s*$)");
                if (pattern.match(trimmed).hasMatch()) {
                    shouldRemove = true;
                    break;
                }
            }
            if (shouldRemove)
                continue; // skip this line entirely
        }

        // Insertion logic
        bool insertedThisLine = false;
        for (const auto& rule : insertionRules) {
            if (rule.targetFunction == functionName) {
                if (rule.afterCall.isEmpty() && !insertedAtStart) {
                    // Insert immediately after opening brace line
                    // Insert only once
                    result << "    " + rule.insertCode;
                    insertedAtStart = true;
                    // continue to process current line normally (don't insert again)
                }
                else if (!rule.afterCall.isEmpty()) {
                    // Insert after a specific call line, e.g. afterCall = "maps\\mp\\_load::main"
                    // Match call ignoring trailing spaces and optional parentheses with or without ;
                    // Use regex to detect calls like "maps\mp\_load::main();" or "maps\mp\_load::main ();"
                    QRegularExpression callPattern("^" + QRegularExpression::escape(rule.afterCall) + R"(\s*\([^)]*\)\s*;?\s*$)");
                    if (callPattern.match(trimmed).hasMatch()) {
                        result << line;
                        result << "    " + rule.insertCode;
                        insertedThisLine = true;
                        break;
                    }
                }
            }
        }

        if (!insertedThisLine)
            result << line;
    }

    return result.join('\n');
}

// Helper function to safely replace function calls globally without messing offsets
QString transformFunctionCalls(const QString& input, const QVector<FunctionMapping>& mappings)
{
    // Pattern: optional "self " + functionName + '(' + args + ')'
    // Capture groups: 1 = "self " or empty, 2 = function name, 3 = args (inside parentheses)
    QRegularExpression callPattern(R"((self\s+)?(\w+)\s*\(([^)]*)\))");

    QString output;
    int lastPos = 0;

    auto it = callPattern.globalMatch(input);
    while (it.hasNext()) {
        auto match = it.next();

        output += input.mid(lastPos, match.capturedStart() - lastPos);

        QString object = match.captured(1);
        QString name = match.captured(2);
        QStringList args = match.captured(3).split(',', Qt::SkipEmptyParts);
        for (QString& arg : args)
            arg = arg.trimmed();

        bool replaced = false;
        for (const auto& mapping : mappings) {
            if (mapping.oldName == name && (mapping.minArgs == -1 || mapping.minArgs <= args.size())) {
                QString replacement;
                if (mapping.transformer)
                    replacement = mapping.transformer(args);
                else
                    replacement = QString("%1%2(%3)").arg(object).arg(mapping.newName).arg(args.join(", "));

                // Avoid double semicolon if original call ended with one
                if (!replacement.endsWith(';'))
                    replacement += ";";

                output += replacement;
                replaced = true;
                break;
            }
        }

        if (!replaced)
            output += match.captured(0);

        lastPos = match.capturedEnd();
    }

    output += input.mid(lastPos);
    return output;
}

// Main function
void ConvertGSCFile(const QString& gscPath)
{
    // 1. Setup your mappings and rules

    QVector<FunctionMapping> functionMappings = {
        { "wait", "Wait", 1, [](const QStringList& args) {
            if (args.isEmpty()) return QString("Wait();"); // safe fallback
            return QString("Wait(%1);").arg(args[0]);
        }}
    };

    // Use QHash for removal rules (functionName -> list of calls to remove)
    QHash<QString, QVector<QString>> removeRules = {
        { "main", { "setExpFog" } }
    };

    // Build insertion rules dynamically since you use gscPath in it
    QVector<InsertionRule> insertionRules = {
    };

    // 2. Read the whole file content
    QFile gscFile(gscPath);
    if (!gscFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open GSC file for reading:" << gscPath;
        return;
    }
    QTextStream in(&gscFile);
    QString content = in.readAll();
    gscFile.close();

    if (content.contains("_load::main()"))
    {
        insertionRules.append({ "main", "", QString("maps\\createart\\%1_art::main();").arg(QFileInfo(gscPath).baseName()) }); // Insert at start of main()
        //insertionRules.append({ "main", "maps\\mp\\_load::main", "thread common_scripts\\_destructible::init();" }); // Insert after _load::main()
    }

    // 3. Parse functions and process them individually
    QRegularExpression functionRegex(R"(^\s*(\w+)\s*\([^)]*\)\s*\n?\s*\{)", QRegularExpression::MultilineOption);
    QString newContent;
    int offset = 0;
    int idx = 0;

    while (idx < content.size()) {
        auto match = functionRegex.match(content, idx);
        if (!match.hasMatch())
            break;

        QString functionName = match.captured(1);
        int funcStart = match.capturedStart();
        int bodyStart = match.capturedEnd();

        // Append content before this function unchanged
        newContent += content.mid(offset, funcStart - offset);

        // Find full function body by matching braces
        int braceDepth = 1;
        int pos = bodyStart;
        while (pos < content.size() && braceDepth > 0) {
            if (content[pos] == '{') braceDepth++;
            else if (content[pos] == '}') braceDepth--;
            pos++;
        }

        // Extract full function block (signature + body)
        QString functionBlock = content.mid(funcStart, pos - funcStart);

        // Process removal and insertion inside this function block
        QString updatedBlock = processFunctionBlock(functionName, functionBlock, removeRules, insertionRules);

        newContent += updatedBlock;

        offset = pos;
        idx = pos;
    }

    // Append remaining content after last function
    newContent += content.mid(offset);

    // 4. Apply function call transformations globally
    QString transformedContent = transformFunctionCalls(newContent, functionMappings);

    // 5. Write back to file
    if (!gscFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        qDebug() << "Failed to open GSC file for writing:" << gscPath;
        return;
    }

    QTextStream out(&gscFile);
    out << transformedContent;
    gscFile.close();

    qDebug() << "Conversion complete for" << gscPath;
}

QStringList findAllGSCFiles(const QString& root)
{
	QStringList result;
	QDirIterator it(root, QStringList() << "*.gsc", QDir::Files, QDirIterator::Subdirectories);
	while (it.hasNext())
		result << it.next();
	return result;
}

void ConvertGSCFiles(const QString& destinationPath)
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
		ConvertGSCFile(path);
	}
	qDebug() << "All GSC files converted in:" << destinationPath;
}