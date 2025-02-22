import random

# used to generate a million token file to benchmark lexing

tokens = [
    "PROGRAM", "CLASS", "END_CLASS", "EXTENDS", "IMPLEMENTS", "INTERFACE", "END_INTERFACE",
    "PROPERTY", "END_PROPERTY", "VAR_INPUT", "VAR_OUTPUT", "VAR", "VAR_CONFIG", "ABSTRACT",
    "FINAL", "METHOD", "CONSTANT", "RETAIN", "NON_RETAIN", "VAR_TEMP", "END_METHOD",
    "PUBLIC", "PRIVATE", "INTERNAL", "PROTECTED", "OVERRIDE", "VAR_GLOBAL", "VAR_IN_OUT",
    "VAR_EXTERNAL", "END_VAR", "END_PROGRAM", "FUNCTION", "END_FUNCTION", "FUNCTION_BLOCK",
    "END_FUNCTION_BLOCK", "TYPE", "STRUCT", "END_TYPE", "END_STRUCT", "ACTIONS", "ACTION",
    "END_ACTION", "END_ACTIONS", "IF", "THEN", "ELSE_IF", "ELSE", "END_IF", "FOR", "TO",
    "BY", "DO", "END_FOR", "WHILE", "END_WHILE", "REPEAT", "UNTIL", "END_REPEAT", "CASE",
    "RETURN", "EXIT", "CONTINUE", "POINTER", "REF", "REFERENCE_TO", "ARRAY", "STRING",
    "WIDE_STRING", "OF", "AT", "END_CASE", "+", "-", "*", "**", "/", "=", "<>", "<", ">",
    "<=", ">=", "&", "^", "MOD", "AND", "OR", "XOR", "NOT"
]

idents = ["myVar", "tempValue", "sensor_1", "x_coord", "loopCounter"]

num_tokens = 1_000_000
out_file = "milltokens.st"

with open(out_file, "w") as f:
    for _ in range(num_tokens):
        if random.random() < 0.20:  # 20% chance we get an identifier
            f.write(random.choice(idents) + " ")
        else:
            f.write(random.choice(tokens) + " ")

print(f"Generated {num_tokens} tokens in {out_file}")
