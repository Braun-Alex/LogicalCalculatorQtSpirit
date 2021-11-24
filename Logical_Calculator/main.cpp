#include "logical_calculator_interface.h"
#include <QApplication>
int main(int argc, char *argv[])
{
    QApplication the_application(argc, argv);
    Logical_Calculator_Interface the_interface;
    the_interface.show();
    return the_application.exec();
}
