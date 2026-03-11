1. ✅support assign expression

```js
var a = 1;
a = 2;

var b = 3;

a = b = 4;

var c = 5;

c = a;
```

2. ✅support function params LOAD and STORE

```js
func add(a, b){
    print(a + b);
}

add:
  LOAD #0
  LOAD #1
  ADD
  CALL print
```

3.✅support global variable LOAD and STORE

```js
var a = 1;

func main(){
    print(a);
}

func main(){
    var a = 2;
    print(a);
}

func show(a){
    print(a);
}

```


4.✅ support function return

```js
func add(a, b){
    return a + b;
}
```

5.✅ support if and while statement

```js
if(a > 1){

}else{

}

while(a < 100){
    print(a);
    a = a + 1;
}
```

6. ✅ support include 

```js
include("std");

func main(){
    stdFunction();
}
```

7. ✅ support comment

```js
include("std");

// Single-line comment:

/*
multi-line comment
*/
func main(){
    stdFunction();
}
```

8. ✅ support access.

```js
func main(){
    var a = {"name":"lily"};
    print(a.name);
}
```

9.  📌 remove register

- remove ei register
- remove cvalues register


10.  📌 support access functin type member

```js
func k(a){
    print("k");
    print(a);
}
func main(){
    var a = {"name":"lily", "action": k};
    a.k(10);
}
```

11. 📌 for member functio, support `this` keyword.

```js
func k(a){
    print(a);
    print(this.name);
}
func main(){
    var a = {"name":"lily", "action": k};
    a.k(10);
}
```

12. 📌 recycle include and include twice.

```js
include("std");
include("std"); // error!
// Single-line comment:

/*
multi-line comment
*/
func main(){
    stdFunction();
}
```

13. ✅ support function call recycle

```js
func a(){
    a();
}

```

