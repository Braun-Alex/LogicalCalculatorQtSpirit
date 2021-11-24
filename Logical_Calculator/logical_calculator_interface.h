#pragma once
#ifndef LOGICAL_CALCULATOR_INTERFACE_H
#define LOGICAL_CALCULATOR_INTERFACE_H
#include <QMainWindow>
#include <QTableWidgetItem>
#include <QFile>
#include <QFileDialog>
#include <QJsonObject>
#include <QJsonDocument>
#include <QMessageBox>
#include <algorithm>
#include <iterator>
#include <boost/fusion/adapted.hpp>
#include <boost/variant.hpp>
#include <boost/spirit/home/x3.hpp>
#include <boost/functional/overloaded_function.hpp>
namespace x3=boost::spirit::x3;
QT_BEGIN_NAMESPACE
namespace Ui { class Logical_Calculator_Interface; }
QT_END_NAMESPACE
class Logical_Calculator_Interface:public QMainWindow
{
Q_OBJECT
public:
Logical_Calculator_Interface(QWidget* the_widget=nullptr);
~Logical_Calculator_Interface();
private slots:
void ChangeExpression();
void ChangeStatus();
void ChangeFontStatus();
void AddRow();
void RemoveRow();
void AddColumn();
void RemoveColumn();
void OpenFile();
void SaveFile();
void ChangeOpacity();
private:
    int the_current_rows=7,the_current_columns=3;
    const QStringList the_list={"A","B","C","D","E",
                         "F","G","H","I","J",
                         "K","L","M","N","O",
                         "P","Q","R","S","T",
                         "U","V","W","X","Y","Z"};
    QFont the_halloween_font,the_default_font,another_font;
    QMessageBox the_error_message,the_warning_message;
    QTableWidgetItem* the_item=nullptr;
    QFile the_file;
    QFileDialog the_dialog;
    QJsonDocument the_document;
    QJsonObject the_object;
    QVariantMap the_map;
    std::vector<std::map<int,QString>> the_rows_data,the_columns_data;
    Ui::Logical_Calculator_Interface *ui;
    QString EvaluateValue(const QString &entered_expression);
    bool RunUnitTests();
    bool RunEvaluatingTests();
    bool RunRowsAndColumnsTests();
    bool RunInterfaceTests();
    void RunEvaluatingTest(const QString &the_expression,const QString
                           &the_right_result);
    void RunRowsAndColumnsTest(int the_number,int the_right_number);
};
#endif
