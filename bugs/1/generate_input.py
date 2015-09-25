def generate_input(n):
	s = "GET /{path} HTTP/1.0"
	s = s.replace("{path}","".join(['a' for i in xrange(int(n))]))
	return s

print generate_input(raw_input())