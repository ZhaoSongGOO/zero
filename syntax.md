
```
program    = { func_define } ;

statement  = var_decl ";" 
           | expr_stmt ";"
           | ret_stmt ";"
           | block_stmt
           | if_stmt 
           | while_stmt
           | include_stmt ";" ;
           
ret_stmt = "return" [expression];

include_stmt = "include" "(" string ")"

if_stmt = "if" "(" expression ")" block_stmt ["else" (if_stmt | block)] 

while_stmt = "while" "(" expression ")" block_stmt;

func_define = "func" string"("[params]")" "{" {statement} "}";

params = identifier {"," identifier} ;

var_decl   = "var" identifier "=" expression ;

expr_stmt  = expression ; 

expression = assign_expr ;

assign_expr = identifier "=" assign_expr 
            | logic_or ;

# level 1
logic_or = logic_and { "||" logic_and };

# level 2
logic_and = equality { "&&" equality };

# level 3
equality = comparison [("==" | "!=") comparison];

# level 4
comparison = assignment [(">" | "<" | ">=" | "<=" ) assignment];

# level 5
assignment = term { ( "+" | "-" ) term } ;

# level 6
term       = factor { ( "*" | "/" ) factor } ;

# level 7
factor     = [ "!" | "-" ] primary_call ; 

# level 8
primary_call = primary { "(" [ arg_list ] ")" };

primary    = identifier {"." string} | number | string | bool |array_list | object_list | "(" expression ")" ;

arg_list   = expression { "," expression } ;

array_lit  = "[" [ expression { "," expression } ] "]" ;

object_lit = "{" [ kv_pair { "," kv_pair } ] "}" ;

kv_pair    = string ":" expression ;
```