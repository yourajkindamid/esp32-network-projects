import os
import sys
import math
import time

import pygame

FILE_NAME = "scan_data.txt"

SCREEN_WIDTH = 800
SCREEN_HEIGHT = 800 
RADAR_RADIUS = 350
RADAR_CENTER_X = SCREEN_WIDTH / 2
RADAR_CENTER_Y = SCREEN_HEIGHT / 2 

MAX_DIST = 120.0           
POINT_INTERVAL_MS = 200   

BG_COLOR = (0, 20, 0)
GRID_COLOR = (0, 150, 0)
TEXT_COLOR = (0, 200, 0)
SWEEP_COLOR = (0, 255, 0)
DETECTION_COLOR = (255, 0, 0)

FPS = 60


def load_scan_points(file_path):
    """Load and parse 'angle, distance' lines from the given text file."""
    if not os.path.exists(file_path):
        print(f"Could not find {file_path}.")
        print("Place your scan .txt file next to this script, or update FILE_NAME.")
        sys.exit(1)

    points = []
    with open(file_path, "r") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            values = line.split(",")
            if len(values) != 2:
                continue
            try:
                angle = float(values[0].strip())
                distance = float(values[1].strip())
                points.append((angle, distance))
            except ValueError:
                continue

    print(f"Loaded {len(points)} scan points from {file_path}")
    if not points:
        print("No valid points found -- check the format is 'angle, distance' per line.")
        sys.exit(1)

    return points


def polar_to_screen(angle_deg, distance_px, center_x, center_y):
    """Same convention as the original Processing sketch: angle - 180,
    then standard cos/sin (both use a y-down coordinate system, so the
    math translates directly)."""
    rad = math.radians(angle_deg - 180)
    x = center_x + distance_px * math.cos(rad)
    y = center_y + distance_px * math.sin(rad)
    return x, y


def draw_radar_grid(screen, font_small):
    for i in range(1, 4):
        radius = i * (RADAR_RADIUS / 3.0)
        pygame.draw.circle(screen, GRID_COLOR, (int(RADAR_CENTER_X), int(RADAR_CENTER_Y)), int(radius), 2)

    pygame.draw.circle(screen, GRID_COLOR, (int(RADAR_CENTER_X), int(RADAR_CENTER_Y)), int(RADAR_RADIUS), 2)

    for i in range(8):
        angle = math.radians(i * 45)
        x2 = RADAR_CENTER_X + RADAR_RADIUS * math.cos(angle)
        y2 = RADAR_CENTER_Y + RADAR_RADIUS * math.sin(angle)
        pygame.draw.line(screen, GRID_COLOR, (RADAR_CENTER_X, RADAR_CENTER_Y), (x2, y2), 2)


def draw_text_labels(screen, font_small, font_title, current_angle, current_distance):
    for i in range(1, 4):
        radius_text = i * (RADAR_RADIUS / 3.0)
        label = font_small.render(f"{i * 40} cm", True, TEXT_COLOR)
        screen.blit(label, (RADAR_CENTER_X + radius_text + 5, RADAR_CENTER_Y - 20))

    angle_label = font_small.render(f"Angle: {current_angle} deg", True, TEXT_COLOR)
    screen.blit(angle_label, (20, 20))

    dist_label = font_small.render(f"Distance: {current_distance} cm", True, TEXT_COLOR)
    screen.blit(dist_label, (20, 45))

    title_label = font_title.render("Radar Display", True, TEXT_COLOR)
    screen.blit(title_label, (SCREEN_WIDTH / 2 - 80, 20))


def draw_sweep_line(screen, current_angle):
    x2, y2 = polar_to_screen(current_angle, RADAR_RADIUS, RADAR_CENTER_X, RADAR_CENTER_Y)
    pygame.draw.line(screen, SWEEP_COLOR, (RADAR_CENTER_X, RADAR_CENTER_Y), (x2, y2), 3)


def draw_detected_point(screen, current_angle, current_distance, point_history):
    if 0 < current_distance < MAX_DIST:
        mapped_dist = (current_distance / MAX_DIST) * RADAR_RADIUS
        x, y = polar_to_screen(current_angle, mapped_dist, RADAR_CENTER_X, RADAR_CENTER_Y)
        pygame.draw.circle(screen, DETECTION_COLOR, (int(x), int(y)), 3)
        point_history.append({"x": x, "y": y, "age": 255})


def update_and_draw_history(screen, point_history):
    new_history = []
    for p in point_history:
        color = (0, max(p["age"], 0), 0)
        pygame.draw.circle(screen, color, (int(p["x"]), int(p["y"])), 2)
        p["age"] -= 2
        if p["age"] > 0:
            new_history.append(p)
    point_history[:] = new_history


def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    file_path = os.path.join(script_dir, FILE_NAME)
    scan_points = load_scan_points(file_path)

    pygame.init()
    screen = pygame.display.set_mode((SCREEN_WIDTH, SCREEN_HEIGHT))
    pygame.display.set_caption("Radar Display")
    clock = pygame.time.Clock()

    font_small = pygame.font.SysFont("monospace", 16)
    font_title = pygame.font.SysFont("monospace", 20, bold=True)

    point_index = 0
    current_angle, current_distance = scan_points[0]
    last_advance_time = time.time() * 1000
    point_history = []

    running = True
    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False

        now_ms = time.time() * 1000
        if now_ms - last_advance_time >= POINT_INTERVAL_MS:
            last_advance_time = now_ms
            current_angle, current_distance = scan_points[point_index]
            point_index = (point_index + 1) % len(scan_points)  # loop playback

        screen.fill(BG_COLOR)
        draw_radar_grid(screen, font_small)
        draw_text_labels(screen, font_small, font_title, current_angle, current_distance)
        draw_sweep_line(screen, current_angle)
        draw_detected_point(screen, current_angle, current_distance, point_history)
        update_and_draw_history(screen, point_history)

        pygame.display.flip()
        clock.tick(FPS)

    pygame.quit()


if __name__ == "__main__":
    main()