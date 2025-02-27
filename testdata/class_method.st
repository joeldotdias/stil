CLASS MyClass
    VAR
        x, y : INT;
        temperature: REAL := 38.2;
    END_VAR

    METHOD testMethod
        VAR_INPUT myMethodArg : INT; END_VAR
        VAR myMethodLocalVar : INT; END_VAR

        x := myMethodArg;
        y := 2
    END_METHOD
END_CLASS
