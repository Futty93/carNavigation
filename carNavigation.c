/* dijkstra1.c --- ダイクストラ法第１ステップ */
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <GL/glfw.h>
#include <FTGL/ftgl.h>


#define CrossingNumber 100  /* 交差点数=100 */
#define MaxName         50  /* 最大文字数50文字(半角) */

#define PATH_SIZE     100  /* 経路上の最大の交差点数 */
#define MARKER_RADIUS 0.2  /* マーカーの半径 */

/* 座標変換マクロの定義 */
double ORIGIN_X = 0;
double ORIGIN_Y = 0;
double scale = 8.0; //入力するたびにうざいので小文字に変更


/*ナビの今の状態を管理する変数*/
bool guide = false;
bool destination = false;
bool currentLocation = false;

#ifndef FONT_FILENAME
/* フォントのファイル名 */
#define FONT_FILENAME "/usr/share/fonts/truetype/takao-gothic/TakaoGothic.ttf"
#endif
static FTGLfont *font; /* 読み込んだフォントを差すポインタ */

typedef struct {
    double x, y;             /* 位置 x, y */
} Position;                /* 位置を表す構造体 */

typedef struct {
    int id;                /* 交差点番号 */
    Position pos;          /* 位置を表す構造体 */
    double wait;           /* 平均待ち時間 */
    char jname[MaxName];   /* 交差点名 */
    char ename[MaxName];   /* 交差点名 */
    int points;            /* 交差道路数 */
    int next[5];           /* 隣接する交差点番号 */
    double distance;    /* 基準交差点までの距離：追加 */
    int previous;       /* 基準交差点からの経路（直前の交差点番号）：追加 */
} Crossing;

Crossing cross[CrossingNumber];

/* データを格納する変数の定義 */
//static Crossing cross[CROSSING_SIZE];
//static int path[PATH_SIZE + 1];  /* 経路: 通過する交差点IDを順番に格納したもの */


double rgbValue(int rgb){
	double value;
	value = (double)rgb/255;
	return value;
}

/* 円を描画 */
static void drawCircle(double x, double y, double r) {
    int const N = 24;             /* 円周を24分割して線分で描画することにする */
    int i;

    glBegin(GL_LINE_LOOP);
    for (i = 0; i < N; i++){
        glVertex2d(x + cos(2 * M_PI * i / N) * r,
                   y + sin(2 * M_PI * i / N) * r);
    }
    glEnd();
}

/*マーカーを描画*/
static void drawCarMarker(double x, double y, double r, int goal, bool driving, float rot){

	float angle = -(rot/180)*(M_PI); //度からラジアンに変換

	if(goal != -1 && hypot(cross[goal].pos.x - x,cross[goal].pos.y - y)>0.01 && driving == true){
		glColor3d(1.0, 1.0, 1.0);
		glLineWidth(2);
		drawCircle(x,y,r);
		glColor3d(rgbValue(0),rgbValue(170),rgbValue(255));
    	glBegin(GL_POLYGON);
    	glVertex2d(x-r*sin(angle),y+r*cos(angle));
    	glVertex2d(x-(r/2)*cos(angle)+(r/1.41)*sin(angle),y-(r/2)*sin(angle)-(r/1.41)*cos(angle));
    	glVertex2d(x+(r/3)*sin(angle),y-(r/3)*cos(angle));
    	glColor3d(rgbValue(0),rgbValue(150),rgbValue(200));
    	glBegin(GL_POLYGON);
    	glVertex2d(x-r*sin(angle),y+r*cos(angle));
    	glVertex2d(x+(r/2)*cos(angle)+(r/1.41)*sin(angle),y+(r/2)*sin(angle)-(r/1.41)*cos(angle));
    	glVertex2d(x+(r/3)*sin(angle),y-(r/3)*cos(angle));
    	glEnd();
    	glColor3d(1.0, 1.0, 1.0);
		glLineWidth(4);
		glBegin(GL_LINE_LOOP);
		glVertex2d(x-r*sin(angle),y+r*cos(angle));
    	glVertex2d(x-(r/2)*cos(angle)+(r/1.41)*sin(angle),y-(r/2)*sin(angle)-(r/1.41)*cos(angle));
    	glVertex2d(x+(r/3)*sin(angle),y-(r/3)*cos(angle));
    	glVertex2d(x+(r/2)*cos(angle)+(r/1.41)*sin(angle),y+(r/2)*sin(angle)-(r/1.41)*cos(angle));
    	//glEnd();
    }else{
    	glColor3d(1.0, 1.0, 1.0);
		glLineWidth(4);
		drawCircle(x,y,r/2);
		glColor3d(rgbValue(0),rgbValue(170),rgbValue(255));
		glBegin(GL_POLYGON);
		for (int i=0;i<12;i++){
		    glVertex2d(x + cos(2 * M_PI * i / 12) * (r/2-0.01),
		               y + sin(2 * M_PI * i / 12) * (r/2-0.01));
		}
		glEnd();
    }
    /*glBegin(GL_LINE_LOOP);
    for (i = 0; i < N; i++)  
        glVertex2d(x + cos(2 * M_PI * i / N) * r,  hypot(cross[goal].pos.x - x,cross[goal].pos.y - y)<0.01
                   y + sin(2 * M_PI * i / N) * r);*/
    glEnd();
}

/* 文字列を描画 */
static void drawOuttextxy(double x, double y, char const *text) {
    double const scale = 0.01;
    glPushMatrix();
    glTranslated(x, y, 0.0);
    glScaled(scale, scale, scale);
    ftglRenderFont(font, text, FTGL_RENDER_ALL);
    glPopMatrix();
}

double getFontSize(double scale){
	double fontSize;
	if(scale<6){
		fontSize = 2*scale + 14;
	}else if(scale<8){
		fontSize = 36;
	}else if(scale<9){
		fontSize = 38;
	}else if(scale<10){
		fontSize = 42;
	}else if(scale<11){
		fontSize = 46;
	}else if(scale<20){
		fontSize = scale + 36;
	}else{
		fontSize = scale * 3;
	}
    return fontSize;
}

void drawRectangle(double topY, double bottomY, double leadingX, double trailingX, int r, int g, int b, int textColor, char *text, double size, double scale){ //指定した位置に指定したサイズの文字やボタンを描画する

	int fontSize;
	fontSize = (int)(getFontSize(scale) * (size/10.0));
	ftglSetFontFaceSize(font, fontSize, fontSize);
	ftglSetFontDepth(font, 0.01);
	ftglSetFontOutset(font, 0, 0.1);
	ftglSetFontCharMap(font, ft_encoding_unicode);
    glColor3d(rgbValue(r), rgbValue(g), rgbValue(b));
	glLineWidth(3);
	glBegin(GL_POLYGON);
    glVertex2d(leadingX, topY);
    glVertex2d(trailingX, topY);
    glVertex2d(trailingX, bottomY);
    glVertex2d(leadingX, bottomY);
    glEnd();
    glColor3d(rgbValue(textColor), rgbValue(textColor), rgbValue(textColor));
    /*if(number != 0){
    	drawOuttextxy(leadingX+(scale/80), bottomY + ((topY - bottomY)/3), cross[number].jname);
    }else{*/
		drawOuttextxy(leadingX+(scale/80), bottomY + ((topY - bottomY)/3), text);
	//}
	glEnd();
	
}


int map_read(char *filename){
    FILE *fp;
    int i, j;
    int crossing_number;          /* 交差点数 */
    
    fp = fopen(filename, "r");
    if (fp == NULL) {
        perror(filename);
        return -1;
    }
    
    /* はじめに交差点数を読み込む */
    fscanf(fp, "%d", &crossing_number);
    
    for (i = 0; i < crossing_number; i++) {
        fscanf(fp, "%d,%lf,%lf,%lf,%[^,],%[^,],%d",
               &(cross[i].id), &(cross[i].pos.x), &(cross[i].pos.y),
               &(cross[i].wait), cross[i].jname,
               cross[i].ename, &(cross[i].points));
        
        for (j = 0; j < cross[i].points; j++) {
            fscanf(fp, ",%d", &(cross[i].next[j]));
        }
        
    }
    fclose(fp);
    
    /* ファイルから読み込んだ交差点数を返す */
    return crossing_number;
}

/* 道路網の表示 */
static void map_show(int crossing_number, int path[], double size) {
    int i, j;
    double x0, y0, x1, y1;
    
    for (i = 0; i < crossing_number; i++) {     /* 交差点毎のループ */
        x0 = cross[i].pos.x;
        y0 = cross[i].pos.y;
        

        /* 交差点から伸びる道路を描く */
        glColor3d(1.0, 1.0, 1.0);
        glLineWidth(24/size);
        glBegin(GL_LINES);
        for (j = 0; j < cross[i].points; j++) {
            x1 = cross[ cross[i].next[j] ].pos.x;
            y1 = cross[ cross[i].next[j] ].pos.y;
            glVertex2d(x0, y0);
            glVertex2d(x1, y1);
        }
        glEnd();
        
        //経路確定前ならすべての交差点の名前を描く
        if(path[0] == -1){
		    glColor3d(0.6, 0.6, 0.6);
		    drawOuttextxy(x0, y0, cross[i].jname);
        }
        glEnd();
        
        //経路を表示
        
        if(path[0] != -1){
		    glColor3d(rgbValue(255), rgbValue(136), rgbValue(51));
			glLineWidth(40/size);
			glBegin(GL_LINE_STRIP);
			for(j=0;j<20;j++){
				glVertex2d(cross[path[j]].pos.x, cross[path[j]].pos.y);
				if(path[j+1]==-1){
					break;
				}
			}
			glEnd();
		}

        /* 交差点を表す円を描く */
        glLineWidth(1);
        glColor3d(1.0, 0.5, 0.5);
        drawCircle(x0, y0, 0.05);

        glEnd();
    }
    if(path[0] != -1){
    	for(i=0;i<20;i++){
    		glColor3d(0.6, 0.6, 0.6);
		    drawOuttextxy(cross[path[i]].pos.x, cross[path[i]].pos.y, cross[path[i]].jname);
		    if(path[i+1] == -1){
		    	break;
		    }
		    glEnd();
    	}
    }
}

/* サーチとソートの演習のprint_crossを改変，距離を表示できるよう改造してます */
void print_cross(int i){
    int j;
    printf("交差点番号:%2d, 座標(%5.2lf,%5.2lf), 名前: %s ( %s ),",
           cross[i].id, cross[i].pos.x, cross[i].pos.y,
           cross[i].jname,cross[i].ename);
    
    printf("\n    目的地までの距離 %5.1lf, 直前の交差点 :%d, 待ち時間:%5.1lf, 隣接交差点 :%d個 ( ",
           cross[i].distance,cross[i].previous,cross[i].wait, cross[i].points);  /* ここを改造 */
    for(j=0; j<cross[i].points; j++)         /* 交差道路数だけ繰り返し */
    printf("%d ", cross[i].next[j]);
    
    printf(")\n\n");
}

//使わないのでコメントアウト
/*void print_cross_list(int num){  //num個表示 = 表示数制限可能
    int i;
    for(i=0;i < num; i++)
    print_cross(i);      //関数print_crossをnum回呼び出す
}*/

//使わないのでコメントアウト
/*int search_cross(int num){  // 完全一致サーチのプログラムをこの行以降に入力して下さい
    int i;
    int f = -1;
    char input[200];
    printf("交差点名を入力してください(ローマ字): ");
    scanf("%s", input);
    
    puts("");
    for (i = 0; i < num; i++) {
        
        // i番目の交差点名(ename)と入力したinput が一致したら: 考えてください
        if(strstr(cross[i].ename, input) != NULL){
            print_cross(i);  //表示
            f = i;             //見つけた番号保持
        }
    }
    if(f < 0){
        printf("%s はみつかりませんでした\n", input);
    }
    
    return f;  //見つかっていたら、それを、なければ -1 を返します
}*/

/* 交差点 a と b の間の「距離」を与える */
double distance(int a, int b){
    return hypot(cross[a].pos.x-cross[b].pos.x,   /* この式を変えると */
                 cross[a].pos.y-cross[b].pos.y);  /* 評価がかわります */
}

void dijkstra(int crossing_number, int target, int start){
    int i, j, n;
    double d;
    double min_distance;          /* 例によって「最小」を探すための変数 */
    int min_cross = 0;
    int done[CrossingNumber];     /* 確定済み:1 未確定:0 を入れるフラグ */
    
    for(i=0;i<crossing_number;i++){ /* 初期化 */
        cross[i].distance = 1e100;  /* 初期値は有り得ないくらい大きな値 */
        cross[i].previous = -1;     /* 最短経路情報を初期化 */
        done[i] = 0;                /* 全交差点未確定 */
    }
    
    /* ただし、基準の交差点は 0 */
    cross[target].distance = 0;
    
    for(i=0;i<crossing_number;i++){ /* crossing_number回やれば終わるはず */
        /* 最も距離数値の小さな未確定交差点を選定 → サーチのところ参照 */
        min_distance=1e100;
        for(j=0;j<crossing_number;j++){
            if(done[j] == 0){
                if(cross[j].distance + distance(start, j) < min_distance + distance(start, min_cross)){
                   //探索している地点とスタートまでの直線距離を足すことでより効率的に経路を探索できそう
                
                //if(cross[j].distance < min_distance){
                    min_distance = cross[j].distance;
                    min_cross = cross[j].id; //jでもいいがやっていることをわかりやすいように
                }
            }
        }
        
        /* 交差点 min_cross は 確定できる */
        done[min_cross] = 1;
        
        if(min_cross == start){ //スタート地点に到達したら処理を終了する
        	break;
        }
        
        
        /* 確定交差点周りで距離の計算 */
        for(j=0;j<cross[min_cross].points;j++){
        
        	n=cross[min_cross].next[j];
            
            /* 評価指標「距離」の計算 */
            d = distance(min_cross, n) + cross[min_cross].distance;
            
            /* 現在の暫定値と比較して、短いならdistanceとpreviousを更新 */
            if(d < cross[n].distance){
                cross[n].distance = d;
                cross[n].previous = min_cross;
            }
        }
    }
}

/* 最短パス設定 */
int pickup_path(int crossing_number, int start, int goal, int path[], int maxpath){
    int c = start;        
  	int i;

  	path[0]=start;
  	i=1;             
  	while(c != goal){
      	c = cross[c].previous;
      	path[i] = c;
      	i++;
    }
  	path[i] = -1;   
  	return 0;
}

float getAngle(int currentPos, int path[]){
	float previousX;
	float previousY; //一つ前の交差点の座標
	float currentX;
	float currentY; //現在の交差点の座標
	float followX;
	float followY; //次の交差点の座標
	float turn;

	if(currentPos == 0){ //スタート位置にいる時は南の座標を代入
		previousX = cross[path[currentPos]].pos.x;
		previousY = cross[path[currentPos]].pos.y-1;
	}else{
		previousX = cross[path[currentPos-1]].pos.x;
		previousY = cross[path[currentPos-1]].pos.y;
	}
	currentX = cross[path[currentPos]].pos.x;
	currentY = cross[path[currentPos]].pos.y;
	if(path[currentPos + 1] == -1){ //ゴール位置にいる時は遥か北の座標を代入
		followX = cross[path[currentPos]].pos.x;
		followY = cross[path[currentPos]].pos.y+1;
	}else{
		followX = cross[path[currentPos+1]].pos.x;
		followY = cross[path[currentPos+1]].pos.y;
	}
	
	float innerProduct = (followX - currentX)*(previousX - currentX) + (followY - currentY)*(previousY - currentY); //内積の計算
	
	float absoluteValue =  (hypot(followX - currentX,followY - currentY))*(hypot(previousX - currentX,previousY - currentY)); //絶対値の積を計算
	
	float cosValue = innerProduct/absoluteValue;
	
	float angle = acos(cosValue);
	
	float determinant = ((previousX - currentX)*(followY - currentY))-((followX - currentX)*(previousY - currentY));
	
	if(determinant < 0){ //右折だったらAngleを負に入れ替える
		turn = (angle/M_PI)*180 - 180;
	}else{
		turn = 180 - (angle/M_PI)*180;
	}
	
	return turn;
}

int checkClick(int x, int y, double centerX, double centerY, double scale, int num){ //クリックされた座標から次の処理を決定
	double minDistance = 100;
	double distance = 0;
	int closeCross = 0;
	double mapX, mapY; //ウィンドウの座標を地図上の座標にする
	int i;
	
	mapX = ((double)x/700 - 0.5) * scale + centerX;
	mapY = -((double)y/700 - 0.5) * scale + centerY;
	for(i=0;i<num;i++){
		distance = hypot(cross[i].pos.x-mapX, cross[i].pos.y-mapY);
		if(distance < minDistance){
			closeCross = i;
			minDistance = distance;
		}
	}
	return closeCross;
}


int main(void){
    int crossing_number;                          /* 交差点数 */
    int goal=-1,start=-1;
    int path[20];
    int i;
    double fontSize; //文字サイズを管理
    int vehicle_pathIterator = 0;     /* 移動体の経路上の位置 (何個目の道路か) */
    int vehicle_stepOnEdge = 0;       /* 移動体の道路上の位置 (何ステップ目か) */
    double vehicle_x = 100, vehicle_y = 100; /* 移動体の座標 */
    int width, height;
    double x0, y0, x1, y1;
    double distance;
    int steps;
    bool driving = false;
    bool onCross = true;
    float rot = 0;
    float rotAngle = 0;
    int count = 0;
    int frequency = 0;
    
    for(i=0;i<20;i++){
		path[i] = -1;   //すべてのパスを初期化
	}
    
    /* ファイルの読み込み */
    crossing_number = map_read("map2.dat");
    printf("loaded %d crossings\n",crossing_number);
    for(i=0;i<crossing_number;i++){
        cross[i].distance=0;    /* 適当に初期化しておきます for print_cross */
        cross[i].previous=-1;   /* 適当に初期化しておきます for print_cross */
    }
    
    /* 目的地の取得 */
    /*printf("目的地を決定します。");
    goal = search_cross(crossing_number);
    
    if(goal<0){
        return 1;    //目的地決定失敗
    }
    
    printf("現在地を決定します。");
    start = search_cross(crossing_number);
    if(start<0){
        return 1;
    }
    
    n = dijkstra(crossing_number, goal, start);
    //print_cross_list(crossing_number);
    
    if(pickup_path(crossing_number, start, goal, path, 40) < 0){
        return 1;
    }
    
    printf("経路確定しました\n");
    
    driving = true;
    
    i=0;
    while(path[i]>=0){
        printf("%2d %5.1lf %s\n",
               i+1,cross[path[i]].distance,cross[path[i]].jname);
        i++;
    }
    
    printf("\n%d\n\n", n);*/
    
        //グラフィック環境を初期化して、ウィンドウを開く
    glfwInit();
    glfwOpenWindow(700, 700, 0, 0, 0, 0, 0, 0, GLFW_WINDOW); //正方形の画面を開く
    
    while (1) {
    
    	/* 文字列描画のためのフォントの読み込みと設定 */
    	font = ftglCreateExtrudeFont(FONT_FILENAME);
		if (font == NULL) {
		    perror(FONT_FILENAME);
		    fprintf(stderr, "could not load font\n");
		    exit(1);
		}
		//画面サイズに合わせて文字サイズを変更
		if(scale<3){
			fontSize = scale * 8;
		}else if(scale<4){
			fontSize = scale * 7;
		}else if(scale<5){
			fontSize = scale * 6;
		}else if(scale<6){
			fontSize = scale * 5;
		}else if(scale<7){
			fontSize = scale * 3;
		}else{
			fontSize = scale * 2;
		}
		
		ftglSetFontFaceSize(font, fontSize, fontSize);
		ftglSetFontDepth(font, 0.01);
		ftglSetFontOutset(font, 0, 0.1);
		ftglSetFontCharMap(font, ft_encoding_unicode);
    
		
		 /* (ORIGIN_X, ORIGIN_Y) を中心に、scale * scale の範囲の
		     空間をビューポートに投影する */
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		
		glOrtho(ORIGIN_X + scale * -0.5, ORIGIN_X + scale * 0.5,
				ORIGIN_Y + scale * -0.5, ORIGIN_Y + scale * 0.5,
				-1.0, 1.0); //現在地を中心に地図を描画
		

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();             /* それ以外の座標変換は行わない */
		
		
		glTranslatef(ORIGIN_X,ORIGIN_Y,0); //進行方向に合わせて地図を回転する
        glRotatef(rot,0,0,1);
        glTranslatef(-ORIGIN_X,-ORIGIN_Y,0);
		
		if(driving == true){ //ドライブの最中なら
			if(hypot(cross[goal].pos.x - vehicle_x,cross[goal].pos.y - vehicle_y)<0.01){
				if(scale<9){ //ゴールについたらズームアウト
					scale *= 1.05;
					ORIGIN_X = (cross[goal].pos.x + cross[start].pos.x)/2;
					ORIGIN_Y = (cross[goal].pos.y + cross[start].pos.y)/2;
					if(scale>8){
						rot = 0;
						driving = false;
					} //ゴールについてズームアウトが終わったらドライブ終了
				}
		    }else if(scale > 3){ //スタート直後にズームイン
				scale *= 0.95;
			}
		}
		
		/*if(driving == true)
			if(hypot(cross[goal].pos.x - vehicle_x,cross[goal].pos.y - vehicle_y)<0.01 && scale>8){
				driving = false;
				for(i=0;i<20;i++){
					path[i] = -1;   //すべてのパスを初期化
				}
			} //ゴールについてズームアウトが終わったらドライブ終了
		}*/
			
		
		if(driving == false){
			if(glfwGetKey(GLFW_KEY_UP) == GLFW_PRESS){ //矢印キー↑がおされたら移動
		    	ORIGIN_Y += scale/10;
		    }
		    
		    if(glfwGetKey(GLFW_KEY_DOWN) == GLFW_PRESS){ //矢印キー↓がおされたら移動
		    	ORIGIN_Y -= scale/10;
		    }
		    
		    if(glfwGetKey(GLFW_KEY_RIGHT) == GLFW_PRESS){ //矢印キー→がおされたら移動
		    	ORIGIN_X += scale/10;
		    }
		    
		    if(glfwGetKey(GLFW_KEY_LEFT) == GLFW_PRESS){ //矢印キー←がおされたら移動
		    	ORIGIN_X -= scale/10;
		    }
		    
		    if(scale>3){
				if(glfwGetKey(GLFW_KEY_UP) == GLFW_PRESS && 
					glfwGetKey(GLFW_KEY_DOWN) == GLFW_PRESS){ //↑↓同時押しで拡大
					scale -= 0.5;
				}
			}
			if(scale<20){
				if(glfwGetKey(GLFW_KEY_LEFT) == GLFW_PRESS && 
					glfwGetKey(GLFW_KEY_RIGHT) == GLFW_PRESS){ //←→同時押しで縮小
					scale += 0.5;
				}
			}
		}
		
		
        /* Esc が押されるかウィンドウが閉じられたらおしまい */
        if (glfwGetKey(GLFW_KEY_ESC) || !glfwGetWindowParam(GLFW_OPENED))
            break;

        glfwGetWindowSize(&width, &height); /* 現在のウィンドウサイズを取得する */
        glViewport(0, 0, width, height); /* ウィンドウ全面をビューポートにする */

        glClearColor(rgbValue(238), rgbValue(238), rgbValue(221), 0.0f);
        glClear(GL_COLOR_BUFFER_BIT); /* バックバッファを塗り潰す */

		map_show(crossing_number, path, scale);                /* 道路網の表示 */
        
		if(driving == true){    
		    if(onCross == false){
				/* 移動体を進めて座標を計算する */
				if (path[vehicle_pathIterator + 0] != -1 &&
				        path[vehicle_pathIterator + 1] != -1) {
				    /* まだゴールに達していないので、移動体の位置を進める */

				    x0 = cross[path[vehicle_pathIterator + 0]].pos.x;
				    y0 = cross[path[vehicle_pathIterator + 0]].pos.y;
				    x1 = cross[path[vehicle_pathIterator + 1]].pos.x;
				    y1 = cross[path[vehicle_pathIterator + 1]].pos.y;
				    distance = hypot(x1 - x0, y1 - y0);
				    steps = (int)(distance / 0.02);

				    /* 道路上を進んで、座標を更新 */
				    vehicle_stepOnEdge++;
				    vehicle_x = x0 + (x1 - x0) / steps * vehicle_stepOnEdge;
				    vehicle_y = y0 + (y1 - y0) / steps * vehicle_stepOnEdge;
				    ORIGIN_X = vehicle_x;
        			ORIGIN_Y = vehicle_y;

				    if (vehicle_stepOnEdge >= steps) {
				        /* 交差点に達したので次の道路へ入る */
				        vehicle_pathIterator++;
				        vehicle_stepOnEdge = 0;
				        onCross = true;
				    }
				}
			}
		    
		    if(onCross == true){ //交差点についたら回転する
		    	if(count == 0){
		    		rotAngle = getAngle(vehicle_pathIterator, path);
		    		frequency = (int)(rotAngle/3); //θ/10回に分けて回転する
		    		if(frequency<0){
		    			frequency *= -1;
		    		}else if(frequency == 0){
		    			frequency = 1;
		    		}
		    	}
		    	if(count<frequency){
		    		rot += (float)(rotAngle/frequency);
		    		count++;
		    	}
		    	if(count == frequency){
		    		onCross = false;
		    		rotAngle = 0;
		    		frequency = 0;
		    		count = 0;
		    	}
		    }
		}
        
        //glRotatef(rot,0,0,1);
		
        
        /*if(driving == true){
        	ORIGIN_X = vehicle_x;
        	ORIGIN_Y = vehicle_y;
        }*/

        /* 移動体を表示 */
        drawCarMarker(vehicle_x, vehicle_y, MARKER_RADIUS, goal, driving, rot);
        glEnd();
        
        
        if(driving == false){
		    drawRectangle(ORIGIN_Y + scale * -0.44, ORIGIN_Y + scale * -0.46,
		    			  ORIGIN_X + scale * -0.5, ORIGIN_X + scale * -0.3,
		    			  255, 255, 255, 0, "←↓↑→で移動", 10.0, scale);
		    drawRectangle(ORIGIN_Y + scale * -0.46, ORIGIN_Y + scale * -0.48,
		    			  ORIGIN_X + scale * -0.5, ORIGIN_X + scale * -0.3,
		    			  255, 255, 255, 0, "↑↓同時押しで拡大", 10.0, scale);
		    drawRectangle(ORIGIN_Y + scale * -0.48, ORIGIN_Y + scale * -0.5,
		    			  ORIGIN_X + scale * -0.5, ORIGIN_X + scale * -0.3,
		    			  255, 255, 255, 0, "←→同時押しで縮小", 10.0, scale);
		    if(guide == false && destination == false && currentLocation == false){
				drawRectangle(ORIGIN_Y + scale * -0.44, ORIGIN_Y + scale * -0.5,
							  ORIGIN_X + scale * -0.3, ORIGIN_X + scale * -0.1,
							  17, 238, 255, 255, "経路検索", 15.0, scale);
			}
			if(guide == false && destination == true && currentLocation == true){
				drawRectangle(ORIGIN_Y + scale * -0.44, ORIGIN_Y + scale * -0.5,
							  ORIGIN_X + scale * -0.1, ORIGIN_X + scale * 0.1,
							  221, 34, 68, 255, "案内終了", 15.0, scale);
			}
			if(guide == true){
				drawRectangle(ORIGIN_Y + scale * 0.5, ORIGIN_Y + scale * 0.4,
							  ORIGIN_X + scale * -0.1, ORIGIN_X + scale * 0.5,
							  238, 238, 255, 100, "目的地", 12.0, scale);
				drawRectangle(ORIGIN_Y + scale * -0.44, ORIGIN_Y + scale * -0.5,
							  ORIGIN_X + scale * -0.3, ORIGIN_X + scale * -0.1,
							  255, 221, 119, 60, "取り消し", 10.0, scale);
				if(destination == false){
					drawRectangle(ORIGIN_Y + scale * 0.5, ORIGIN_Y + scale * 0.4,
								  ORIGIN_X + scale * 0.05, ORIGIN_X + scale * 0.5,
								  153, 221, 238, 100, "目的地をクリックしてください", 10.0, scale);
				}else if(destination == true){
					drawRectangle(ORIGIN_Y + scale * 0.5, ORIGIN_Y + scale * 0.4,
								  ORIGIN_X + scale * 0.05, ORIGIN_X + scale * 0.5,
								  238, 238, 255, 100, cross[goal].jname, 10.0, scale);
					
					drawRectangle(ORIGIN_Y + scale * 0.49, ORIGIN_Y + scale * 0.41,
								  ORIGIN_X + scale * 0.35, ORIGIN_X + scale * 0.45,
								  156, 156, 156, 238, "キャンセル", 6.0, scale);
					
					drawRectangle(ORIGIN_Y + scale * 0.4, ORIGIN_Y + scale * 0.3,
						  		  ORIGIN_X + scale * -0.1, ORIGIN_X + scale * 0.5,
						  		  238, 238, 255, 100, "現在地", 20.0, scale);
					
					if(currentLocation == false){
						drawRectangle(ORIGIN_Y + scale * 0.4, ORIGIN_Y + scale * 0.3,
									  ORIGIN_X + scale * 0.05, ORIGIN_X + scale * 0.5,
									  221, 170, 221, 100, "現在地をクリックしてください", 5.0, scale);
					}else if(currentLocation == true){
						drawRectangle(ORIGIN_Y + scale * 0.4, ORIGIN_Y + scale * 0.3,
									  ORIGIN_X + scale * 0.05, ORIGIN_X + scale * 0.5,
									  238, 238, 255, 100, cross[start].jname, 7.0, scale);
									  
						drawRectangle(ORIGIN_Y + scale * 0.39, ORIGIN_Y + scale * 0.31,
									  ORIGIN_X + scale * 0.35, ORIGIN_X + scale * 0.45,
									  156, 156, 156, 238, "キャンセル", 8.0, scale);
						drawRectangle(ORIGIN_Y + scale * 0.3, ORIGIN_Y + scale * 0.25,
									  ORIGIN_X + scale * -0.1, ORIGIN_X + scale * 0.5,
									  153, 255, 153, 255, "案内開始", 15.0, scale);
					}
				}
			}
		}    
		
		// マウスのボタンが押されたかを調べる
        if( glfwGetMouseButton(GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS ) {
            int mousex, mousey;
            glfwGetMousePos(&mousex, &mousey);
            if(mousex>(int)(145*((double)width/700)) && mousex<(int)(276*((double)width/700)) && mousey>(int)(662*((double)height/700)) && mousey<(int)(698*((double)height/700))){
            	if(guide == true){
            		//取り消しが押された時に完全に初期状態に戻す
            		destination = false;
            		currentLocation = false;
            		goal = -1;
            		start = -1;
					vehicle_pathIterator = 0;
					vehicle_stepOnEdge = 0;
					rot = 0;
            		for(i=0;i<20;i++){
						path[i] = -1;   //すべてのパスを初期化
					}
					for(i=0;i<crossing_number;i++){
						cross[i].distance=0;    
						cross[i].previous=-1;  
					}
					rotAngle = 0;
					count = 0;
					frequency = 0;
					guide = false;
					usleep(200 * 1000);
				}else{
            		//経路検索が押された時
					guide = true;
					usleep(200 * 1000);
				}
			}else if(guide == false && mousex>(int)(276*((double)width/700)) && mousex<(int)(405*((double)width/700)) && mousey>(int)(662*((double)height/700)) && mousey<(int)(698*((double)height/700))){
            	if(destination == true && currentLocation == true){
            		//案内終了が押された時に完全に初期状態に戻す
            		destination = false;
            		currentLocation = false;
            		goal = -1;
            		start = -1;
					vehicle_pathIterator = 0;
					vehicle_stepOnEdge = 0;
					rot = 0;
            		for(i=0;i<20;i++){
						path[i] = -1;   //すべてのパスを初期化
					}
					for(i=0;i<crossing_number;i++){
						cross[i].distance=0;    
						cross[i].previous=-1;  
					}
					rotAngle = 0;
					count = 0;
					frequency = 0;
				}
            }else if(destination == true && mousex>(int)(597*((double)width/700)) && mousex<(int)(655*((double)width/700)) && mousey>(int)(11*((double)height/700)) && mousey<(int)(60*((double)height/700))){
				//目的地のキャンセルが押された時
				destination = false;
				for(i=0;i<20;i++){
					path[i] = -1;   //すべてのパスを初期化
				}
				goal = -1; //目的地として保存していた場所を削除
				usleep(300 * 1000);
			}else if(currentLocation==true && mousex>(int)(597*((double)width/700)) && mousex<(int)(655*((double)width/700)) && mousey>(int)(80*((double)height/700)) && mousey<(int)(130*((double)height/700))){
				//現在地のキャンセルが押された時
				currentLocation = false;
				for(i=0;i<20;i++){
					path[i] = -1;   //すべてのパスを初期化
				}
				start = -1; //現在地として保存していた場所を削除
				usleep(300 * 1000);
			}else if(guide == true && destination == false){
				//目的地が空の状態で地図上の点が選択された
				goal = checkClick((int)(mousex*(700/(double)width)), (int)(mousey*(700/(double)height)), ORIGIN_X, ORIGIN_Y, scale, crossing_number);
            	if(goal != start){ //目的地と現在地が同じ場合受け付けない
					destination = true;
				}else{
					goal = -1;
				}
			}else if(guide == true && currentLocation == false){
				//現在地が空の状態で地図上の点が選択された
				start = checkClick((int)(mousex*(700/(double)width)), (int)(mousey*(700/(double)height)), ORIGIN_X, ORIGIN_Y, scale, crossing_number);
				if(start != goal){ //目的地と現在地が同じ場合受け付けない
					currentLocation = true;
				}else{
					start = -1;
				}
            }else if(currentLocation == true && mousex>(int)(282*((double)width/700)) && mousex<(int)(695*((double)width/700)) && mousey>(int)(145*((double)height/700)) && mousey<(int)(173*((double)height/700))){
            	//目的地と現在地が決定し、案内開始が押された
				driving = true;
				onCross = true;
				vehicle_pathIterator = 0;
    			vehicle_stepOnEdge = 0;
    			ORIGIN_X = cross[start].pos.x;
    			ORIGIN_Y = cross[start].pos.y;
    			guide = false;
    		}
		}
        
        if(goal != -1 && start != -1){
        	dijkstra(crossing_number, goal, start);
    		if(pickup_path(crossing_number, start, goal, path, 40) < 0){
				return 1;
			}
		}
		
		glfwSwapBuffers();  /* フロントバッファとバックバッファを入れ替える */
        usleep(16 * 1000);  /* 50ミリ秒くらい待つ */
    }

    glfwTerminate();
    return 0;
}

