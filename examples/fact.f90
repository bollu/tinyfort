def fact(i: int): int {
  if i == 0 {
    return 1;
  }
  return i * fact(i -1);
}

def main() : int {
      print(fact(5));
      return 0;
}
