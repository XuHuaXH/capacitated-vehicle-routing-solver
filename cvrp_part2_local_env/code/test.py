import heapq

a = [4, 2, 7, 8, 1, 6, 5, 0, 19]
h = []
for i in a:
    if len(h) < 4:
        heapq.heappush(h, -i)
    elif i < -h[0]:
        heapq.heappop(h)
        heapq.heappush(h, -i)
    print(h)

print(-h[0])
heapq.heappop(h)
print(-h[0])
heapq.heappop(h)
print(-h[0])
heapq.heappop(h)
print(-h[0])