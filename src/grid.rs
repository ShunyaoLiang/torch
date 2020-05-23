use std::ops;
use std::slice;

pub struct Grid<T: Clone> {
    buf: Vec<T>,
    lines: usize,
    cols: usize,
}

impl<T: Clone> Grid<T> {
    pub fn new((lines, cols): (usize, usize), value: T) -> Self {
        Self { buf: vec![value; lines * cols], lines, cols, }
    }

    pub fn resize(&mut self, (new_lines, new_cols): (usize, usize), value: T) {
        self.buf.resize(new_lines * new_cols, value);
        self.lines = new_lines;
        self.cols = new_cols;
    }

    pub fn fill(&mut self, value: T) {
        for t in &mut self.buf {
            *t = value.clone();
        }
    }

    pub fn size(&self) -> (usize, usize) {
        (self.lines, self.cols)
    }

    pub fn in_bounds(&self, (line, col): (usize, usize)) -> bool {
        line < self.lines && col < self.cols
    }

    pub fn windows(&self, size: usize) -> slice::Windows<T> {
        self.buf.windows(size)
    }
}

impl<T: Clone> ops::Index<(usize, usize)> for Grid<T> {
    type Output = T;

    fn index(&self, (line, col): (usize, usize)) -> &Self::Output {
        if self.in_bounds((line, col)) {
            &self.buf[line * self.cols + col]
        } else {
            panic!(
                "pos out of bounds: size is {:?} but pos is {:?}",
                (self.lines, self.cols),
                (line, col)
            )
        }
    }
}

impl<T: Clone> ops::IndexMut<(usize, usize)> for Grid<T> {
    fn index_mut(&mut self, (line, col): (usize, usize)) -> &mut Self::Output {
        if self.in_bounds((line, col)) {
            &mut self.buf[line * self.cols + col]
        } else { 
            panic!(
                "pos out of bounds: size is {:?} but pos is {:?}",
                (self.lines, self.cols),
                (line, col)
            )
        }
    }
}
