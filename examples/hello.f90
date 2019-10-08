def fact(i: int): int {
  if i == 0 {
    return 1;
  }
  return i * fact[i -1];
}

def foo(n: int): int {
  x : int;
  x = 1;

  i: int;
  i = 1;


  
  while i <= n {
    i = i + 1;
    x = x + i * i;

    if x == 200 {
      return x;
    }
  }

  return 1000;
}

def main(i:int) : int {
   x : int;
   a : int;
   b : int;
   y : int[10];
   z: int[3, 4];
   s: int;

   print[fact[5]];

   while x == 0 {
       x = x + 1;
   }

   if x <= 5 {
      print[5];
   } elif x <= 7 {
     print[7];
   } else {
     print[9];
   }

   x = 0;
   while x <= 9 {
     y[x] = x;
     x = x + 1;
   }

   x = 0;
   while x <= 9 {
       print[y[x]]; x = x + 1;
   }

   a = 0; 
   while a <= 2 {
     b = 0;
     while b <= 3 {
       z[a, b] = a+b;
       b = b + 1;
     }
     a = a + 1;
   }

   a = 0;
   while a <= 2 {
     print[88];
     b = 0;
     while b <= 3 {
       print[z[a, b]];
       b = b + 1;
     }
     a = a + 1;
   }

   return 0;

}

