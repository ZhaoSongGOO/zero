
```
program    = { statement } ;

statement  = var_decl ";" 
           | expr_stmt ";" ;

var_decl   = "var" identifier "=" expression ;

expr_stmt  = expression ; 

expression = assignment ;
assignment = term { ( "+" | "-" ) term } ;
term       = factor { ( "*" | "/" ) factor } ;

factor     = primary { "(" [ arg_list ] ")" } ; 

primary    = identifier | number | string | bool |array_list | object_list | "(" expression ")" ;

arg_list   = expression { "," expression } ;

array_lit  = "[" [ expression { "," expression } ] "]" ;

object_lit = "{" [ kv_pair { "," kv_pair } ] "}" ;

kv_pair    = string ":" expression ;
```