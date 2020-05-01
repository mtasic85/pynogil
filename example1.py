N = 100_000_000

a = list(range(N))
b = list(range(N))
# c = [0] * N
# c = [a[i] + b[i] for i in range(N)]
c = [x + y for x, y in zip(a, b)]
s = sum(c)
print(s)
