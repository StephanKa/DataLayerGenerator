#include <include/datalayer.h>// for Temperature, test, test4, CyclicGroup

int main()
{
    DefaultGroup.printDatapoints();
    CyclicGroup.printDatapoints();

    Testify::test.set(42);

    Dispatcher.printStructure();
    return 0;
}
