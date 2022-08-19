import math
def factor_int(n):
    return sorted([item for sublist in [[i, int(n/i)] for i in range(1, n) if n % i == 0] for item in sublist])

print(factor_int(10))
