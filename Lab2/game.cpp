#include <cstdio>
#include <iostream>
#include <chrono>
#include <cstring>
#include <deque>
#include <map>
#include "mpi.h"

#define ROWS    7
#define COLS    7

#define DEPTH   7

#define WAKE    0
#define WHAT    1
#define WAIT    2
#define STOP    3
#define TASK    4
#define RESULT  5

#define P 1
#define C 2

/*
 * Klasa koja predstavlja jedan stupac na ploči za igru
 * Ima onoliko elemenata u polju koliko ima redaka ROWS
 * get - vraća koji je element na poziciji position u stupcu
 * set - postavlja element na poziciji position u vrijednost igrača
 * lastPosition - vraća indeks zadnje zauzete pozicije u stupcu
 * isMovePossible - vraća je li moguće dodati element u stupac (tj. ima li mjesta u stupcu, je li pun)
 */
class BoardColumn {
private:
    int column[ROWS]{};

public:
    BoardColumn() {
        for (auto &i : column) {
            i = 0;
        }
    }

    int get(int position) {
        return column[ROWS - 1 - position];
    }

    void set(int position, int player) {
        column[ROWS - 1 - position] = player;
    }

    int lastPosition() {
        for (int i = ROWS - 1; i >= 0; i--) {
            if (get(i) > 0) {
                return i;
            }
        }
        return -1;
    }

    bool isMovePossible() {
        return lastPosition() < ROWS - 1;
    }
};

/*
 * Klasa koja predstavlja ploču za igranje
 * isPositionValid - vraća je li određena pozicija unutar igraće ploče
 * get - vraća element na poziciji (x, y)
 * nextPosition - vraća sljedeću slobodnu poziciju u stupcu x
 * put - stavlja vrijednost igrača player u stupac x i vraća pobjeđuje li igrač player tim potezom
 * isWin - vraća dolazi li do pobjede
 * printGameBoard - ispisuje ploču za igranje na ekran
 */
class GameBoard {
public:
    BoardColumn columns[COLS];

    GameBoard() = default;

    static bool isPositionValid(int x, int y) {
        return x >= 0 && x < COLS && y >= 0 && y < ROWS;
    }

    int get(int x, int y) {
        return columns[x].get(y);
    }

    int nextPosition(int x) {
        return columns[x].lastPosition() + 1;
    }

    bool put(int x, int player) {
        int y = nextPosition(x);
        columns[x].set(y, player);
        return isWin(x, y);
    }

    bool isWin(int x, int y) {
        int init = get(x, y);

        if (init == 0) {
            return false;
        }

        int pairs = 0;
        bool f = true, s = true;

        for (int i = 1; i < 4; i++) {
            if (f && isPositionValid(x, y + i) && get(x, y + i) == init) pairs++;
            else f = false;
            if (s && isPositionValid(x, y - i) && get(x, y - i) == init) pairs++;
            else s = false;
        }

        if (pairs >= 3) {
            return true;
        }

        pairs = 0;
        f = true, s = true;

        for (int i = 1; i < 4; i++) {
            if (f && isPositionValid(x + i, y) && get(x + i, y) == init) pairs++;
            else f = false;
            if (s && isPositionValid(x - i, y) && get(x - i, y) == init) pairs++;
            else s = false;
        }

        if (pairs >= 3) {
            return true;
        }

        pairs = 0;
        f = true, s = true;

        for (int i = 1; i < 4; i++) {
            if (f && isPositionValid(x + i, y + i) && get(x + i, y + i) == init) pairs++;
            else f = false;
            if (s && isPositionValid(x - i, y - i) && get(x - i, y - i) == init) pairs++;
            else s = false;
        }

        if (pairs >= 3) {
            return true;
        }

        pairs = 0;
        f = true, s = true;

        for (int i = 1; i < 4; i++) {
            if (f && isPositionValid(x + i, y - i) && get(x + i, y - i) == init) pairs++;
            else f = false;
            if (s && isPositionValid(x - i, y + i) && get(x - i, y + i) == init) pairs++;
            else s = false;
        }

        return pairs >= 3;
    }

    void printGameBoard() {
        for (int j = ROWS - 1; j >= 0; j--) {
            for (int i = 0; i < COLS; i++) {
                int t = get(i, j);
                switch (t) {
                    case 0:
                        printf("=");
                        break;
                    case P:
                        printf("P");
                        break;
                    case C:
                        printf("C");
                        break;
                    default:
                        break;
                }
            }
            printf("\n");
        }
    }
};

/*
 * Klasa za spremanje poteza
 * emplace_back - dodavanje vrijednosti na zadnju poziciju
 */
class Moves {
private:
    int length;
    int position[10]{};

public:
    Moves() {
        length = 0;
    }

    bool operator<(const Moves &moves) const {
        if (this->length != moves.length) {
            return this->length < moves.length;
        } else {
            for (int i = 0; i < this->length; i++) {
                if (this->position[i] != moves.position[i]) {
                    return this->position[i] < moves.position[i];
                }
            }
            return false;
        }
    }

    void emplace_back(int value) {
        position[length] = value;
        length++;
    }
};

/*
 * Klasa za pojedini zadatak koji se treba obraditi
 */
class Task {
public:
    GameBoard board;
    Moves moves;
    int nextPlayer{};

    Task() = default;

    Task(GameBoard board, Moves moves, int nextPlayer) {
        this->board = board;
        this->moves = moves;
        this->nextPlayer = nextPlayer;
    }
};

/*
 * Klasa za rješenje zadatka
 */
class Result {
public:
    double value{};
    Moves moves;

    Result() = default;
};

/*
 * Klasa za MPI poruku koja se šalje između procesa
 * type - tip poruke
 * size - veličina poruke
 * content - sadržaj poruke
 */
class Message {
public:
    int type;
    int size;
    char content[256];
};

void generateMessage(Message &message, int type) {
    message.type = type;
    message.size = 0;
    memset(message.content, 0, sizeof(message.content));
}

void generateTaskMessage(Message &message, void *content, int size) {
    message.type = TASK;
    message.size = size;
    memcpy(message.content, content, size);
}

void generateResultMessage(Message &message, void *content, int size) {
    message.type = RESULT;
    message.size = size;
    memcpy(message.content, content, size);
}

/*
 * Metoda koja vraća vrijednost drugog igrača u odnosu na zadanog 
 */
int otherPlayer(int player) {
    return 3 - player;
}

/*
 * Metoda za generiranje zadataka
 */
void generateTasks(GameBoard board, int player, int move, int depth, Moves moves, std::deque<Task> &queue) {
    if (move != -1) {
        if (board.put(move, player)) {      // kraj igre GAME OVER
            return;
        }
        moves.emplace_back(move);            // dodaj potez
    }

    if (depth < 2) {
        for (int position = 0; position < COLS; position++) {       // za svaki stupac na ploči
            if (board.columns[position].isMovePossible()) { // ako je moguć taj potez (za taj stupac)
                int other = otherPlayer(player);
                generateTasks(board, other, position, depth + 1, moves, queue); // rekurzivni poziv
            }
        }
    } else {
        int other = otherPlayer(player);
        Task task(board, moves, other);                 // stvori zadatak
        queue.emplace_back(task);                       // dodaj zadatak u red
    }
}

/*
 * Metoda za određivanje vrijednosti stanja
 */
double stateValue(GameBoard board, int player, int move, int depth) {
    if (move != -1) {
        if (board.put(move, player)) { // ako igra završava, ako je potez pobjednički
            if (player == C) {                    // pobjeđuje računalo
                return 1;
            } else {                                    // pobjeđuje igrač
                return -1;
            }
        }
    }
    if (depth < DEPTH) {
        double sum = 0;     // suma vrijednosti
        double value;       // vrijednost stanja
        int numOfMoves = 0; // broj poteza
        for (int position = 0; position < COLS; position++, numOfMoves++) {
            if (board.columns[position].isMovePossible()) { // ako je potez moguć
                int other = otherPlayer(player); // indeks drugog igrača
                value = stateValue(board, other, position, depth + 1);  // rekurzivni poziv
                if (player == C && value == 1) {  // ako pobjeđuje računalo i vrijednost sljedećeg stanja je 1
                    return 1;
                }
                if (player == P && value == -1) { // ako pobjeđuje igrač i vrijednost sljedećeg stanja je -1
                    return -1;
                }

                sum += value;
            }
        }

        return sum / numOfMoves;

    } else {
        return 0;       // inače je vrijednost stanja 0 na najvećoj dubini poziva
    }
}

/*
 * Metoda za određivanje vrijednosti poteza
 */
double moveValue(GameBoard board, int player, int move, int depth, Moves moves, std::map<Moves, double> &results) {
    if (board.put(move, player)) {// ako igrač pobjeđuje ovim potezom
        if (player == C) {
            return 1;
        } else {
            return -1;
        }
    }
    moves.emplace_back(move);
    if (depth < 2) {
        double sum = 0;
        double value;
        int numOfMoves = 0;
        for (int position = 0; position < COLS; position++, numOfMoves++) {
            if (board.columns[position].isMovePossible()) { // je li moguć potez
                int other = otherPlayer(player); // indeks drugog igrača
                value = moveValue(board, other, position, depth + 1, moves, results); // rekurzivni poziv
                if (player == C && value == 1) { // ako pobjeđuje računalo
                    return 1;
                }
                if (player == P && value == -1) { // ako pobjeđuje igrač
                    return -1;
                }

                sum += value;
            }
        }

        return sum / numOfMoves;
    } else {
        return results[moves];
    }
}

/*
 * Mezoda za određivanje poteza računala
 */
int computerMove(GameBoard board, int numProcs) {
    MPI_Status mpiStatus;
    Message message{};
    Moves moves;
    Result result;
    Task task;
    std::map<Moves, double> results;
    std::deque<Task> queue;
    int workers = numProcs - 1;
    auto start = std::chrono::steady_clock::now();

    generateTasks(board, P, -1, 0, moves, queue); // generiraj zadatke u red zadataka

    generateMessage(message, WAKE);  // buđenje radnika
    MPI_Bcast(&message, sizeof(Message), MPI_BYTE, 0, MPI_COMM_WORLD);  // slanje poruke (broadcast) svim radnicima

    while (workers > 0 || not queue.empty()) { // dok se ne odrade svi zadatci

        // primanje poruke od radnika
        MPI_Recv(&message, sizeof(Message), MPI_BYTE, MPI_ANY_SOURCE, MPI_ANY_TAG,
                MPI_COMM_WORLD, &mpiStatus);

        // ako je primljena poruka tipa RESULT
        // spremi rezultat
        if (message.type == RESULT) {
            memcpy(&result, message.content, message.size);
            results[result.moves] = result.value;
        }

        // red zadataka nije prazan, ima zadataka
        if (not queue.empty()) {
            task = queue.front();   // izvadi jedan iz reda
            queue.pop_front();
            generateTaskMessage(message, &task, sizeof(task));   // generiraj poruku za zadatak
            MPI_Send(&message, sizeof(Message), MPI_BYTE, mpiStatus.MPI_SOURCE, 0,
                     MPI_COMM_WORLD); // pošalji poruku
        } else {
            workers--;
            generateMessage(message, WAIT);  // generiraj poruku za čekanje
            MPI_Send(&message, sizeof(Message), MPI_BYTE, mpiStatus.MPI_SOURCE, 0,
                    MPI_COMM_WORLD); // pošalji
        }
    }

    // odredi najbolju vrijednost i najbolji potez
    int bestMove = -1; // inicijalizacija varijabli
    double bestValue = -10;
    double currentValue;
    for (int position = 0; position < COLS; position++) {     // za svaki potez
        if (board.columns[position].isMovePossible()) {  // je li potez moguć
            Moves moves;
            currentValue = moveValue(board, C, position, 1, moves, results);    // izračunaj vrijednost poteza
            printf("%.3lf ", currentValue); // ispiši vrijednost poteza

            if (currentValue > bestValue) {       // ako je vrijednost poteza bolja od dosadašnje najbolje, zapamti
                bestValue = currentValue;
                bestMove = position;
            }
        } else if (board.columns[position].lastPosition() == ROWS - 1) {   // nije moguć potez
            printf("- ");
        }
    }
    printf("\n");
    auto end = std::chrono::steady_clock::now();
    auto diff = end - start;        // računanje trajanja računanja (za mjerenja)
//    printf(" Trajanje : %.0lf ms\n", std::chrono::duration <double, std::milli> (diff).count());
    return bestMove;        // vrati najbolji potez
}

/*
 * Metoda koju izvršava voditelj
 */
void master(int numProcs) {
    Message message{};
    GameBoard board;
    int move;
    bool gameOver = false;

    while (not gameOver) {        // ponavljaj sve dok igra ne završi
        board.printGameBoard(); // iscrtaj ploču za igranje

        scanf("%d", &move); // potez igrača
        gameOver = board.put(move, P);

        if (gameOver) {
            // GAME OVER, igrač pobjeđuje
            break;
        }
        board.printGameBoard();     // ploča se iscrtava i nakon poteza igrača

        move = computerMove(board, numProcs);   // potez računala
        gameOver = board.put(move, C);  // gameOver = je li računalo pobijedilo?
    }

    board.printGameBoard(); // iscrtavanje ploče za igranje
    generateMessage(message, STOP); // generiraj poruku za kraj izvođenja
    MPI_Bcast(&message, sizeof(Message), MPI_BYTE, 0, MPI_COMM_WORLD);  //slanje poruke (broadcast) svim radnicima
}

/*
 * Metoda koju izvodi svaki radnik
 */
void worker() {
    MPI_Status mpiStatus;
    Task task;
    Message message{};
    Result result;
    while (true) { // ponavljaj do završetka igre
        MPI_Bcast(&message, sizeof(Message), MPI_BYTE, 0, MPI_COMM_WORLD); // čekaj na poruku voditelja

        // ako je primljena poruka STOP, kraj
        if (message.type == STOP) {
            break;
        }

        generateMessage(message, WHAT);       // generiranje poruka WHAT, traženje zadatka
        MPI_Send(&message, sizeof(Message), MPI_BYTE, 0, 0, MPI_COMM_WORLD); // slanje poruke
        while (true) {  // ponavljaj dok ima zadataka
            MPI_Recv(&message, sizeof(Message), MPI_BYTE, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &mpiStatus); // primi poruku

            if (message.type == TASK) {
                memcpy(&task, message.content, message.size);   // zapiši sadržaj poruke
                result.moves = task.moves;
                int other = otherPlayer(task.nextPlayer);   // indeks drugog igrača
                result.value = stateValue(task.board, other, -1, 0);   // odredi vrijednost stanja
                generateResultMessage(message, &result, sizeof(result));    // generiraj poruku za rezultat
                MPI_Send(&message, sizeof(Message), MPI_BYTE, 0, 0, MPI_COMM_WORLD); // slanje poruke
            } else if (message.type == WAIT) {
                break;  // primljena je poruka za čekanje, nema više zadataka -> ČEKAJ poruku za nove zadatke / poruku za kraj
            }
        }
    }

}

int main(int argc, char **argv) {
    int numProcs, myRank;

    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
    MPI_Comm_size(MPI_COMM_WORLD, &numProcs);

//    printf("Moj rank je %d od %d procesa!\n", myRank, numProcs);

    if (myRank != 0) {
        worker();
    } else {
        master(numProcs);
    }
    MPI_Finalize();

    return 0;
}
