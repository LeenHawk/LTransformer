int isValid(int number, int *puzzle, int row, int column) {
    int i=0;
    int sectorRow = 3*(row/3);
    int sectorCol = 3*(column/3);
    int row1 = (row+2)%3;
    int row2 = (row+4)%3;
    int col1 = (column+2)%3;
    int col2 = (column+4)%3;

    /* Check for the value in the given row and column */
    for (i=0; i<9; i++) {
        if (puzzle[i*9+column] == number) return false;
        if (puzzle[row*9+i] == number) return false;
    }

    /* Check the remaining four spaces in this sector */
    if(puzzle[ (row1+sectorRow)*9+col1+sectorCol] == number) return false;
    if(puzzle[ (row2+sectorRow)*9+col1+sectorCol] == number) return false;
    if(puzzle[ (row1+sectorRow)*9+col2+sectorCol] == number) return false;
    if(puzzle[ (row2+sectorRow)*9+col2+sectorCol] == number) return false;
    return true;
}

bool r_sudoku_aux(int *puzzle, int row=0, int column=0)
{
    if( 9==row ) {
        return true;
    }
    // Is this element already set?  If so, we don't want to
    // change it.  Recur immediately to the next cell.
    if( puzzle[row*9+column] ){
        if( column==8 ){
            return r_sudoku_aux(puzzle, row+1, 0);
        }else{
            return r_sudoku_aux(puzzle, row, column+1);
        }
    }
    for(int nextNum=1; nextNum<10; nextNum++)
    {
        if(isValid(nextNum, puzzle, row, column)){
            // Temporarily bind this entry to nextNum
            puzzle[row*9+column] = nextNum;
            if(column == 8){
                if(r_sudoku_aux(puzzle, row+1, 0))
                    return true;
            }else{
                if(r_sudoku_aux(puzzle, row, column+1))
                    return true;
            }
            // We failed to find a valid value for this, undo
            puzzle[row*9+column] = 0;
        }
    }
    // Have to backtrack
    return false;
}



bool r_sudoku(int *puzzle)
{
    return r_sudoku_aux(puzzle, 0, 0);
}
