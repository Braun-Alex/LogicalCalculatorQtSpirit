#include "logical_calculator_interface.h"
#include "./ui_logical_calculator_interface.h"
const QString FALSE_EXP="Хиба",TRUE_EXP="Істина";
using Expression=boost::variant<double,
                boost::recursive_wrapper<struct BinaryExpression>,
                boost::recursive_wrapper<struct FunctionCall>>;
struct BinaryExpression
{
    enum OperatorType {Plus,Minus,Mul,Div,Equal,Less,Greater};
    Expression left_operand;
    std::vector<std::pair<OperatorType,Expression>> operators_and_operands;
};
BOOST_FUSION_ADAPT_STRUCT(BinaryExpression,left_operand,operators_and_operands)
struct FunctionCall
{
    std::string function;
    std::vector<Expression> arguments;
};
BOOST_FUSION_ADAPT_STRUCT(FunctionCall,function,arguments)
x3::rule<class identifier,std::string> const identifier;
x3::rule<class not_binary_expression,Expression> const not_binary_expression;
x3::rule<class function_call,FunctionCall> const function_call;
x3::rule<class binary_expression_1,BinaryExpression> const binary_expression_1;
x3::rule<class binary_expression_2,BinaryExpression> const binary_expression_2;
x3::rule<class binary_expression_3,BinaryExpression> const binary_expression_3;
x3::rule<class expression,Expression> const expression;
struct Binary_Operator:x3::symbols<BinaryExpression::OperatorType> {
    explicit Binary_Operator(int precedence)
    {
        switch (precedence)
        {
            case 1:
                add("=",BinaryExpression::Equal);
                add("<",BinaryExpression::Less);
                add(">",BinaryExpression::Greater);
                return;
            case 2:
                add("+",BinaryExpression::Plus);
                add("-",BinaryExpression::Minus);
                return;
            case 3:
                add("*",BinaryExpression::Mul);
                add("/",BinaryExpression::Div);
                return;
        }
        throw std::runtime_error("Невідомий пріоритет: "
                                 +std::to_string(precedence)+"!");
    }
};
auto const identifier_def=x3::raw[+(x3::alpha|'_')];
auto const not_binary_expression_def=
          x3::double_
          | '(' >> expression >> ')'
          | function_call
;
auto const function_call_def=identifier>>'('>>(expression%',')>>')';
auto const binary_expression_1_def=binary_expression_2>>*(Binary_Operator(1)>>binary_expression_2);
auto const binary_expression_2_def=binary_expression_3>>*(Binary_Operator(2)>>binary_expression_3);
auto const binary_expression_3_def=not_binary_expression>>*(Binary_Operator(3)>>not_binary_expression);
auto const expression_def=binary_expression_1;
BOOST_SPIRIT_DEFINE(
        identifier,
        not_binary_expression,
        function_call,
        binary_expression_1, binary_expression_2, binary_expression_3,
        expression)
Expression Parse(const std::string &the_value)
{
    auto the_iterator=the_value.begin();
    Expression result;
    bool IsAllRight=phrase_parse(the_iterator,the_value.end(),expression,x3::space,result);
    if (IsAllRight&&the_iterator==the_value.end()) return result;
    throw std::runtime_error(std::string("Помилка парсингу на: '")+*the_iterator+"'!");
}
double Evaluate_Binary_Expression(BinaryExpression::OperatorType the_operator_type,
                                  double a, double b)
{
    switch (the_operator_type)
    {
        case BinaryExpression::Plus: return a+b;
        case BinaryExpression::Minus: return a-b;
        case BinaryExpression::Mul: return a*b;
        case BinaryExpression::Div: if (b==0) throw std::runtime_error(
                    "Ділення на нуль!"); return a/b;
        case BinaryExpression::Equal: if (a==b) return 1.0; return 0.0;
        case BinaryExpression::Less: if (a<b) return 1.0; return 0.0;
        case BinaryExpression::Greater: if (a>b) return 1.0; return 0.0;
        default: throw std::runtime_error("Невідомий оператор!");
    }
}
double Evaluate(Expression the_expression)
{
    auto the_visitor=boost::make_overloaded_function(
            [](double x)
            {
                return x;
            },
            [](const FunctionCall &the_expression)->double
            {
                auto argument_at=[&the_expression](size_t i)
                {
                    return Evaluate(the_expression.arguments.at(i));
                };
                if (the_expression.function=="not")
                {
                    double bool_value=argument_at(0);
                    if ((bool_value!=0.0&&bool_value!=1.0)
                            ||the_expression.arguments.size()!=1) throw
                    std::runtime_error("Некоректна \"not\" функція!");
                    if (bool_value==0.0) return 1.0;
                    return 0.0;
                }
                if (the_expression.function=="min")
                {
                    if (the_expression.arguments.size()!=2)
                        throw std::runtime_error("Некоректний min()!");
                    return std::min(argument_at(0),argument_at(1));
                }
                if (the_expression.function=="max")
                {
                    if (the_expression.arguments.size()!=2)
                        throw std::runtime_error("Некоректний max()!");
                    return std::max(argument_at(0),argument_at(1));
                }
                std::vector<double> the_values;
                size_t the_size=the_expression.arguments.size();
                the_values.reserve(the_size);
                for (size_t i=0; i<the_size; ++i)
                    the_values.push_back(argument_at(i));
                if (the_expression.function=="mmin")
                {
                    if (the_expression.arguments.empty())
                        throw std::runtime_error("Некоректний mmin()!");
                    return *std::min_element(the_values.begin(),
                                              the_values.end());
                }
                if (the_expression.function=="mmax")
                {
                    if (the_expression.arguments.empty())
                        throw std::runtime_error("Некоректний mmax()!");
                    return *std::max_element(the_values.begin(),
                                              the_values.end());
                }
                throw std::runtime_error("Невідома функція!");
            },
            [](const BinaryExpression &the_expression)->double
            {
                auto the_left_value=Evaluate(the_expression.left_operand);
                for (auto &&the_operator_and_the_operand:the_expression.operators_and_operands)
                {
                    auto the_right_value=Evaluate(the_operator_and_the_operand.second);
                    the_left_value=Evaluate_Binary_Expression(the_operator_and_the_operand.first,
                    the_left_value,the_right_value);
                }
                return the_left_value;
            });
    return boost::apply_visitor(the_visitor,the_expression);
}
Logical_Calculator_Interface::Logical_Calculator_Interface(
        QWidget* the_widget):QMainWindow(the_widget),
        ui(new Ui::Logical_Calculator_Interface)
{
    ui->setupUi(this);
    ui->Value_table->hide();
    ui->Information_about_author->hide();
    ui->Information_about_project->hide();
    ui->Expression_table->horizontalHeader()->setFont(
                ui->Expression_table->verticalHeader()->font());
    ui->Value_table->horizontalHeader()->setFont(
                ui->Value_table->verticalHeader()->font());
    the_item=ui->Expression_table->horizontalHeaderItem(0);
    ui->Value_table->horizontalHeader()->setVisible(true);
    the_halloween_font=ui->Halloween_font->font();
    the_default_font=ui->Default_font->font();
    another_font=ui->Another_font->font();
    ui->Halloween_font->hide();
    ui->Default_font->hide();
    ui->Another_font->hide();
    ui->Information->hide();
    ui->Open->setFont(the_default_font);
    ui->Save->setFont(the_default_font);
    the_error_message.setFont(the_default_font);
    the_error_message.setWindowTitle("Помилка відкриття файлу");
    the_error_message.setIcon(QMessageBox::Critical);
    the_error_message.setMinimumSize(400,300);
    the_error_message.setText("Відкритий файл не є JSON файлом!");
    the_error_message.setButtonText(1,"Зрозуміло");
    if (!RunUnitTests())
    {
    the_warning_message.setFont(the_default_font);
    the_warning_message.setWindowTitle(
                "Виявлення помилкової поведінки юніт-тестами");
    the_warning_message.setIcon(QMessageBox::Warning);
    the_warning_message.setMinimumSize(400,300);
    the_warning_message.setText(
    "Виявлено вразливості. Додаток може працювати некоректно");
    the_warning_message.setButtonText(1,"Зрозуміло");
    the_warning_message.show();
    }
    the_rows_data.resize(the_current_rows+1);
    the_columns_data.resize(the_current_columns+1);
}
void Logical_Calculator_Interface::ChangeExpression()
{
int the_row=ui->Expression_table->currentItem()->row(),
    the_column=ui->Expression_table->currentItem()->column();
const QString &entered_text=ui->Expression_table->currentItem()->text();
if (ui->Value_table->item(the_row,the_column)==nullptr)
ui->Value_table->setItem(the_row,the_column,new QTableWidgetItem());
if (!entered_text.isEmpty())
{
the_rows_data[the_row][the_column]=entered_text;
the_columns_data[the_column][the_row]=entered_text;
ui->Value_table->item(the_row,the_column)->setText(
EvaluateValue(entered_text));
}
else
{
if (the_rows_data[the_row].contains(the_column))
{
    the_rows_data[the_row].erase(the_column);
    the_columns_data[the_column].erase(the_row);
}
ui->Value_table->item(the_row,the_column)->setText("");
}
int the_size=the_rows_data.size();
for (int i=0; i<the_size; ++i)
for (auto the_iterator=the_rows_data[i].begin();
the_iterator!=the_rows_data[i].end(); ++the_iterator)
ui->Value_table->item(i,the_iterator->first)->
setText(EvaluateValue(the_iterator->second));
}
void Logical_Calculator_Interface::ChangeStatus()
{
if (ui->State->currentText()=="Значення")
{
ui->Expression_table->hide();
ui->Value_table->show();
}
else
{
ui->Expression_table->show();
ui->Value_table->hide();
}
}
void Logical_Calculator_Interface::ChangeFontStatus()
{
if (ui->Font_state->currentText()=="Стандартна тема")
{
ui->Menu->setFont(the_default_font);
ui->About->setFont(the_default_font);
ui->Help->setFont(the_default_font);
ui->File->setFont(the_default_font);
ui->About_author->setFont(the_default_font);
ui->About_project->setFont(the_default_font);
ui->Adding_column->setFont(the_default_font);
ui->Adding_row->setFont(the_default_font);
ui->Removing_column->setFont(the_default_font);
ui->Removing_row->setFont(the_default_font);
ui->State->setFont(the_default_font);
ui->Author_information->setFont(the_default_font);
ui->Project_information->setFont(the_default_font);
ui->Open->setFont(the_default_font);
ui->Save->setFont(the_default_font);
ui->Font_state->setFont(the_default_font);
ui->Expression_table->setFont(the_default_font);
ui->Value_table->setFont(the_default_font);
ui->Help->setFont(the_default_font);
ui->Opacity_value->setFont(the_default_font);
ui->Expression_table->horizontalHeader()->
        setFont(the_default_font);
ui->Value_table->horizontalHeader()->
        setFont(the_default_font);
the_error_message.setFont(the_default_font);
the_warning_message.setFont(the_default_font);
}
else
{
ui->Menu->setFont(another_font);
ui->About->setFont(another_font);
ui->Help->setFont(another_font);
ui->File->setFont(another_font);
ui->About_author->setFont(another_font);
ui->About_project->setFont(another_font);
ui->Adding_column->setFont(another_font);
ui->Adding_row->setFont(another_font);
ui->Removing_column->setFont(another_font);
ui->Removing_row->setFont(another_font);
ui->State->setFont(another_font);
ui->Author_information->setFont(another_font);
ui->Project_information->setFont(another_font);
ui->Open->setFont(another_font);
ui->Save->setFont(another_font);
ui->Help->setFont(another_font);
ui->Font_state->setFont(another_font);
ui->Opacity_value->setFont(another_font);
the_error_message.setFont(another_font);
the_warning_message.setFont(another_font);
ui->Expression_table->setFont(the_halloween_font);
ui->Value_table->setFont(the_halloween_font);
ui->Expression_table->horizontalHeader()->
        setFont(the_halloween_font);
ui->Value_table->horizontalHeader()->
        setFont(the_halloween_font);
}
}
void Logical_Calculator_Interface::AddRow()
{
if (the_current_rows==0) ui->Removing_row->setDisabled(false);
++the_current_rows;
ui->Expression_table->insertRow(the_current_rows);
ui->Value_table->insertRow(the_current_rows);
the_rows_data.resize(the_current_rows+1);
}
void Logical_Calculator_Interface::RemoveRow()
{
ui->Expression_table->removeRow(the_current_rows);
ui->Value_table->removeRow(the_current_rows);
--the_current_rows;
if (the_current_rows==0) ui->Removing_row->setDisabled(true);
int the_size=the_current_rows+1;
the_rows_data.resize(the_size);
for (int i=0; i<the_current_columns; ++i)
    if (the_columns_data[i].contains(the_size))
    {
        the_columns_data[i].erase(the_size);
        if (the_columns_data[i].empty()) the_columns_data[i].clear();
    }
for (int i=0; i<the_size; ++i)
    for (auto the_iterator=the_rows_data[i].begin();
         the_iterator!=the_rows_data[i].end(); ++the_iterator)
        ui->Value_table->item(i,the_iterator->first)->
                setText(EvaluateValue(the_iterator->second));
}
void Logical_Calculator_Interface::AddColumn()
{
if (the_current_columns==0) ui->Removing_column->setDisabled(false);
++the_current_columns;
ui->Expression_table->insertColumn(the_current_columns);
ui->Value_table->insertColumn(the_current_columns);
ui->Expression_table->setHorizontalHeaderLabels(the_list);
ui->Value_table->setHorizontalHeaderLabels(the_list);
ui->Expression_table->horizontalHeaderItem(the_current_columns)->
        setFont(the_item->font());
ui->Value_table->horizontalHeaderItem(the_current_columns)->
        setFont(the_item->font());
if (the_current_columns==25) ui->Adding_column->setDisabled(true);
the_columns_data.resize(the_current_columns+1);
}
void Logical_Calculator_Interface::RemoveColumn()
{
if (the_current_columns==25) ui->Adding_column->setDisabled(false);
ui->Expression_table->removeColumn(the_current_columns);
ui->Value_table->removeColumn(the_current_columns);
--the_current_columns;
if (the_current_columns==0) ui->Removing_column->setDisabled(true);
int the_size=the_current_columns+1;
the_columns_data.resize(the_size);
for (int i=0; i<the_current_rows; ++i)
    if (the_rows_data[i].contains(the_size))
    {
        the_rows_data[i].erase(the_size);
        if (the_rows_data[i].empty()) the_rows_data[i].clear();
    }
for (int i=0; i<the_size; ++i)
    for (auto the_iterator=the_columns_data[i].begin();
         the_iterator!=the_columns_data[i].end(); ++the_iterator)
        ui->Value_table->item(i,the_iterator->first)->
                setText(EvaluateValue(the_iterator->second));
}
void Logical_Calculator_Interface::OpenFile()
{
QString the_path=the_dialog.getOpenFileUrl().path();
if (the_path.isEmpty()) return;
the_path.erase(the_path.begin(),std::next(the_path.begin()));
the_file.setFileName(the_path);
the_file.open(QIODeviceBase::ReadOnly);
the_document=the_document.fromJson(the_file.readAll());
the_file.close();
if (!the_document.isObject())
{
the_error_message.show();
return;
}
int the_size=the_rows_data.size();
for (int i=0; i<the_size; ++i)
for (const auto &[the_key,the_value]:the_rows_data[i])
{
ui->Expression_table->item(i,the_key)->setText("");
ui->Value_table->item(i,the_key)->setText("");
}
the_object=the_document.object();
int the_row_count=the_object.value("rows").toInt()-1;
if (the_row_count<the_current_rows)
    while (the_row_count!=the_current_rows) RemoveRow();
else if (the_row_count>the_current_rows)
    while (the_row_count!=the_current_rows) AddRow();
int the_column_count=the_object.value("columns").toInt()-1;
if (the_column_count<the_current_columns)
    while (the_column_count!=the_current_columns) RemoveColumn();
else if (the_column_count>the_current_columns)
    while (the_column_count!=the_current_columns) AddColumn();
the_rows_data.clear();
the_columns_data.clear();
the_rows_data.resize(the_current_rows);
the_columns_data.resize(the_current_columns);
the_map=the_object.value("table").toObject().toVariantMap();
auto the_iterator=the_map.constBegin();
while (the_iterator!=the_map.constEnd())
{
std::string the_key=the_iterator.key().toStdString();
auto the_divider=std::find(the_key.begin(),the_key.end(),'|');
int the_row=QString::fromStdString(
            std::string(the_key.begin(),the_divider)).toInt(),
    the_column=QString::fromStdString(std::string(std::next(the_divider),
                                             the_key.end())).toInt();
--the_row;
--the_column;
if (ui->Expression_table->item(the_row,the_column)==nullptr)
ui->Expression_table->setItem(the_row,the_column,new QTableWidgetItem());
if (ui->Value_table->item(the_row,the_column)==nullptr)
ui->Value_table->setItem(the_row,the_column,new QTableWidgetItem());
const QString &entered_text=the_iterator->value<QString>();
ui->Expression_table->item(the_row,the_column)->
            setText(entered_text);
if (!entered_text.isEmpty())
{
the_rows_data[the_row][the_column]=entered_text;
the_columns_data[the_column][the_row]=entered_text;
ui->Value_table->item(the_row,the_column)->setText(
EvaluateValue(entered_text));
}
else ui->Value_table->item(the_row,the_column)->setText("");
++the_iterator;
}
}
void Logical_Calculator_Interface::SaveFile()
{
QString the_path=the_dialog.getSaveFileUrl(nullptr,QString(),
QUrl(),"json",nullptr,QFileDialog::Options(),QStringList()).path();
if (the_path.isEmpty()) return;
the_path+=".json";
the_path.erase(the_path.begin(),std::next(the_path.begin()));
the_file.setFileName(the_path);
the_object.insert("rows",the_current_rows+1);
the_object.insert("columns",the_current_columns+1);
int the_size=the_rows_data.size();
for (int i=0; i<the_size; ++i)
    for (auto the_iterator=the_rows_data[i].begin();
         the_iterator!=the_rows_data[i].end(); ++the_iterator)
the_map[QString::fromStdString(std::to_string(i+1))+
QString::fromStdString("|")+
QString::fromStdString(std::to_string(the_iterator->first+1))]=
the_iterator->second;
the_object.insert("table",QJsonObject::fromVariantMap(the_map));
the_document.setObject(the_object);
the_file.open(QIODeviceBase::WriteOnly);
the_file.write(the_document.toJson());
the_file.close();
}
void Logical_Calculator_Interface::ChangeOpacity()
{
qreal the_opacity_value=qreal(1)-qreal(ui->Opacity->value())/qreal(125);
this->setWindowOpacity(the_opacity_value);
std::string the_copy=std::to_string(the_opacity_value*100);
the_copy.erase(std::find(the_copy.begin(),the_copy.end(),'.'),
               the_copy.end());
ui->Opacity_value->setText("Прозорість: "+QString::
                           fromStdString(the_copy)+"%");
}
QString Logical_Calculator_Interface::EvaluateValue(
        const QString &entered_expression)
{
std::string the_copy=entered_expression.toStdString();
try
{
auto the_letter_predicate=[&the_copy](char the_symbol)
{
return the_symbol>='A'&&the_symbol<='Z';
};
auto the_number_predicate=[&the_copy](char the_symbol)
{
return the_symbol>='1'&&the_symbol<='9';
};
auto the_iterator=the_copy.begin();
while (the_iterator!=the_copy.end())
{
the_iterator=std::find_if(the_iterator,the_copy.end(),
                       the_letter_predicate);
if (the_iterator==the_copy.end()) break;
int the_column=*the_iterator-'A';
auto another_symbol=std::find_if(the_iterator,the_copy.end(),
                       the_number_predicate);
if (another_symbol==the_copy.end()) throw
    std::runtime_error("Хибна назва комірки!");
while (the_number_predicate(*another_symbol)
       &&another_symbol!=the_copy.end()) another_symbol++;
std::string the_number(++the_iterator,another_symbol);
int the_row=QString::fromStdString(the_number).toInt()-1;
if (ui->Value_table->
    item(the_row,the_column)==nullptr||
    ui->Value_table->
    item(the_row,the_column)->text().isEmpty()) throw
    std::runtime_error("Комірка невалідна!");
if (ui->Value_table->item(the_row,the_column)->text()=="Хиба")
    *std::prev(the_iterator)=char(48);
else if (ui->Value_table->item(the_row,the_column)->text()=="Істина")
    *std::prev(the_iterator)=char(49);
else throw std::runtime_error("Комірка невалідна!");
while (the_iterator!=another_symbol)
    *(the_iterator++)=char(32);
}
Expression the_expression=Parse(the_copy);
double the_result=Evaluate(the_expression);
if (the_result!=0.0&&the_result!=1.0) throw
    std::runtime_error("Вираз не є логічним!");
if (the_result==0.0) return "Хиба";
return "Істина";
}
catch(const std::runtime_error &the_error)
{
return QString::fromStdString(the_error.what());
}
}
bool Logical_Calculator_Interface::RunUnitTests()
{
return RunEvaluatingTests()&&RunRowsAndColumnsTests()&&
        RunInterfaceTests();
}
bool Logical_Calculator_Interface::RunEvaluatingTests()
{
try
{
RunEvaluatingTest("3+3=6","Істина");
RunEvaluatingTest("mmin(3,3,3)=mmax(3,3,3)","Істина");
RunEvaluatingTest("6+5*3+6=27","Істина");
RunEvaluatingTest("(5>3)=(not(max(15,33)>29)=(mmax(15,33,111)<mmin(111,111,111)))",
                  "Істина");
RunEvaluatingTest("27*3/9+3>15","Хиба");
RunEvaluatingTest("sin(3)>3","Невідома функція!");
RunEvaluatingTest("min(3,2,1)>3","Некоректний min()!");
RunEvaluatingTest("max(3,2,1)>3","Некоректний max()!");
RunEvaluatingTest("5/0>119","Ділення на нуль!");
return true;
}
catch(const std::logic_error&)
{
return false;
}
}
bool Logical_Calculator_Interface::RunRowsAndColumnsTests()
{
try
{
for (int i=0; i<15; ++i) AddColumn();
RunRowsAndColumnsTest(the_current_columns,18);
for (int i=0; i<333; ++i) AddRow();
RunRowsAndColumnsTest(the_current_rows,340);
for (int i=0; i<335; ++i) RemoveRow();
RunRowsAndColumnsTest(the_current_rows,5);
AddRow();
RunRowsAndColumnsTest(the_current_rows,6);
for (int i=0; i<16; ++i) RemoveColumn();
RunRowsAndColumnsTest(the_current_columns,2);
AddRow();
RunRowsAndColumnsTest(the_current_rows,7);
AddColumn();
RunRowsAndColumnsTest(the_current_columns,3);
return true;
}
catch(const std::out_of_range&)
{
return false;
}
}
bool Logical_Calculator_Interface::RunInterfaceTests()
{
try
{
if (!ui->Information->isHidden()||
ui->Expression_table->isHidden()||
!ui->Value_table->isHidden()||
!ui->Information_about_author->isHidden()||
!ui->Information_about_project->isHidden())
    throw std::logic_error("");
if (ui->State->currentText()!="Вираз")
    throw std::invalid_argument("");
if (this->windowOpacity()!=1.0)
    throw std::out_of_range("");
if (this->windowTitle()!="Логічний калькулятор"||
    ui->Information_about_project->windowTitle()!="Коротко про проєкт"||
    ui->Information->windowTitle()!="Допомога користувачу")
    throw std::invalid_argument("");
if (!ui->Adding_row->isEnabled()||
    !ui->Adding_column->isEnabled()||
    !ui->Removing_row->isEnabled()||
    !ui->Removing_column->isEnabled()||
    !ui->Value_table->isEnabled())
    throw std::logic_error("");
return true;
}
catch(const std::exception&)
{
return false;
}
}
void Logical_Calculator_Interface::RunEvaluatingTest(const QString &the_expression,
const QString &the_right_result)
{
if (EvaluateValue(the_expression)!=the_right_result)
    throw std::logic_error("");
}
void Logical_Calculator_Interface::RunRowsAndColumnsTest(int the_number,
int the_right_number)
{
if (the_number!=the_right_number)
   throw std::out_of_range("");
}
Logical_Calculator_Interface::~Logical_Calculator_Interface()
{
    delete ui;
}
