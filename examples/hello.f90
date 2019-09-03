def fib(i: int): int {
  if i == 0 {
    return 1;
  }
  return i * fib[i -1];
}

def main(i:int) : void {
   x : int;
   y : int[10, 20];
   x = 1;
   print[x+1];

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

}

