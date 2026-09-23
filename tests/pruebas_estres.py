import random
import sys

N = int(sys.argv[1]) if len(sys.argv) > 1 else 10000
random.seed(42)

for i in range(1, N + 1):
    candidatos = list(range(max(1, i - 20), i))

    k = min(len(candidatos), random.randint(0, 3))
    deps = random.sample(candidatos, k=k) if k > 0 else []
    deps_str = ", ".join(str(d) for d in deps)

    print(f"{i} : act_{i} : 5 : {deps_str}")