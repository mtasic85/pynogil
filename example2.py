from queue import Queue
from threading import Thread

N = 100_000_000
N_THREADS = 10

q = Queue()
a = list(range(N))
b = list(range(N))


def work(sub_a, sub_b):
    sub_c = [x + y for x, y in zip(sub_a, sub_b)]
    sub_c = sum(sub_c)
    q.put(sub_c)


threads = []

for i in range(N_THREADS):
    s = i * (N // N_THREADS)
    e = s + (N // N_THREADS)
    sub_a = a[s:e]
    sub_b = b[s:e]
    t = Thread(target=work, args=(sub_a, sub_b))
    threads.append(t)

for t in threads:
    t.start()

for t in threads:
    t.join()

sub_cs = []

while not q.empty():
    sub_c = q.get()
    sub_cs.append(sub_c)
    q.task_done()

s = sum(sub_cs)
print(s)
