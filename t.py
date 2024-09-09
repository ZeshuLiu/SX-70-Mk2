class Solution:
    def mostFrequentPrime(self, mat) -> int:
        di = {}
        m = len(mat)
        n = len(mat[0])
        for i in range(m):
            for j in range(n):
                a = mat[i][j]
                # down
                for k in range(i+1, m):
                    a = a*10 + mat[k][j]
                    if self.isPrime(a):
                        if a in di:
                            di[a] +=1
                        else:
                            di[a] = 1

                a = mat[i][j]
                # up
                for k in range(i-1, -1, -1):
                    a = a*10 + mat[k][j]
                    if self.isPrime(a):
                        if a in di:
                            di[a] +=1
                        else:
                            di[a] = 1
                
                # right
                a = mat[i][j]
                for k in range(j + 1, n):
                    a = a*10 + mat[i][k]
                    if self.isPrime(a):
                        if a in di:
                            di[a] +=1
                        else:
                            di[a] = 1

                # left
                a = mat[i][j]
                for k in range(j -1, -1, -1):
                    a = a*10 + mat[i][k]
                    if self.isPrime(a):
                        if a in di:
                            di[a] +=1
                        else:
                            di[a] = 1
                    
                # d-r
                a = mat[i][j]
                ii = i
                jj = j
                while ii < m-1 and jj <n-1:
                    ii += 1
                    jj += 1
                    a = a*10 + mat[ii][jj]
                    if self.isPrime(a):
                        if a in di:
                            di[a] +=1
                        else:
                            di[a] = 1
                
                # d-l
                a = mat[i][j]
                ii = i
                jj = j
                while ii < m-1 and jj > 0:
                    ii += 1
                    jj -= 1
                    a = a*10 + mat[ii][jj]
                    if self.isPrime(a):
                        if a in di:
                            di[a] +=1
                        else:
                            di[a] = 1
                
                # u-l
                a = mat[i][j]
                ii = i
                jj = j
                while ii > 0 and jj > 0:
                    ii -= 1
                    jj -= 1
                    a = a*10 + mat[ii][jj]
                    if self.isPrime(a):
                        if a in di:
                            di[a] +=1
                        else:
                            di[a] = 1
                # u-r
                a = mat[i][j]
                ii = i
                jj = j
                while ii > 0 and jj < n-1:
                    ii -= 1
                    jj += 1
                    a = a*10 + mat[ii][jj]
                    if self.isPrime(a):
                        if a in di:
                            di[a] +=1
                        else:
                            di[a] = 1

        mx = 0
        ans = -1
        for i in di:
            if di[i] > mx:
                ans = i
                mx = di[i]
            if di[i] == mx:
                ans = max(ans, i)
        print(di)
        return ans

    def isPrime(self, n):
        if n <= 2:
            return True
        for i in range(2, int(n**0.5)+2):
            if n % i == 0:
                return False
        return True
    
a = Solution()
print(a.mostFrequentPrime([[9,3,8],[4,2,5],[3,8,6]]))