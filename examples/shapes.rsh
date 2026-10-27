struct Point:
    x: float
    y: float

impl Point:
    fn length(self) -> float:
        return sqrt(self.x * self.x + self.y * self.y)

fn main():
    let p = Point(3.0, 4.0)
    print("length = {p.length():.2f}")
