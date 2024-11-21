#include<iostream>
#include<vector>
#include<algorithm>
using namespace std;

/* 
	Simulating distance vector routing with the Bellman Ford Algorithm
	we define routing_table[i][u][v][t] = shortest path from u to v via t after i iterations
	so routing_table[i][u][][][] represents i-th version of router u's routing table
	TESTING INPUT	
	6 12
	1 2 21
	1 3 7
	1 5 6
	1 6 5
	2 3 16
	2 4 8
	2 6 4
	3 4 3
	3 5 12
	4 5 9
	4 6 14
	5 6 2
*/

const int MAXN = 58; //Max number of edges
const int INF = 1e9+7;

int n,m, routing_table[MAXN][MAXN][MAXN][MAXN], aux[MAXN][MAXN][MAXN];

void update_aux(int i){//aux[i][u][v] = shortest path from u to v we know in iteration i
	for(int u=1; u<=n; u++){
		for(int v=1; v<=n; v++){
			aux[i][u][v] = aux[i-1][u][v];
			for(int t=1; t<=n; t++){
				aux[i][u][v] = min(aux[i][u][v], aux[1][u][t] + aux[i-1][t][v]);
			}
		}
	}
}

int main(){
	
	cin>>n>>m;
	
	//we set versions 0 and 1 of the routing tables (BASE CASES)
	for(int u=1; u<=n; u++){
		for(int v=1; v<=n; v++){
			aux[0][u][v] = (u==v) ? 0 : INF;
			aux[1][u][v] = aux[0][u][v];
			for(int t=1; t<=n; t++){
				routing_table[0][u][v][t] = (u==v) ? 0 : INF;
				routing_table[1][u][v][t] = routing_table[0][u][v][t];
			}
		}
	}
	
	for(int i=0; i<m; i++){
		int u,v,w; cin>>u>>v>>w;
		routing_table[1][u][v][v] = min(routing_table[1][u][v][v], w);
		routing_table[1][v][u][u] = min(routing_table[1][v][u][u], w);
		aux[1][u][v] = min(aux[1][u][v], w);
		aux[1][v][u] = min(aux[1][v][u], w);
	}
	
	for(int i=2; i<=n; i++){//we compute versions 2,...,n with Bellman-Ford DP
		update_aux(i);
		for(int u=1; u<=n; u++){//let's compute routing_table[i][u][][]
			for(int v=1; v<=n; v++){
				for(int t=1; t<=n; t++){//can I improve path u->v going via t ?
					routing_table[i][u][v][t] = min(routing_table[i-1][u][v][t], aux[1][u][t] + aux[i-1][t][v]);
				}
			}
			
		}
	}
	
	for(int i=0; i<=n; i++){
		cout<<"DISTANCE VECTOR A VERSION "<<i<<endl;
		for(int t=2; t<=n; t++){
			cout<<"via "<<(char)('A'+t-1)<<":";
			for(int v=2; v<=n; v++){
				cout<<routing_table[i][1][v][t]<<" ";
			}
			cout<<endl;
		}
		cout<<"-------------------------"<<endl;
	}
}