1. support function params LOAD and STORE

```js
func add(a, b){
    print(a + b);
}

add:
  LOAD_BP -0
  LOAD_BP -1
  ADD
  CALL print
```

2. support global variable LOAD and STORE

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

3. support assign expression

```js
var a = 1;
a = 2;

var b = 3;

a = b = 4;

var c = 5;

c = a;
```



4. support function return

```js
func add(a, b){
    return a + b;
}
```

5. support if and while statement

```js
if(a > 1){

}else{

}

while(a < 100){
    print(a);
    a = a + 1;
}
```
