#include "gesture.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>

void print_picture_callback(int step) {
    if (step == -1) {
        std::cout << "PICTURE: Gesture complete (global)\n";
    } else {
        std::cout << "PICTURE: At step index " << step << "\n";
    }
}

int main() {
    int signal_index = 1;

    // TEST 1: Threshold-only, some steps with per-step picture
    std::vector<Gesture::Step> sequence = {
        // Step 0: Threshold, move higher, value=10.0, reset if lower than 5.0, take picture
        {
            Gesture::Step::StepType::Threshold,                          // step_type
            Gesture::Step::MoveType::Higher,                             // move_to_next_type
            10.0,                                                        // value
            0.0, 0.0, 0,                                                 // min_value, max_value, min_duration_ms
            Gesture::Step::ResetCondition{
                Gesture::Step::ResetCondition::Type::Lower, 5.0          // reset
            },
            std::nullopt,                                                // entered_range_time
            true                                                         // take_picture_at_the_end
        },
        // Step 1: Threshold, move lower, value=8.0, reset by timeout after 5000ms, take picture
        {
            Gesture::Step::StepType::Threshold,
            Gesture::Step::MoveType::Lower,
            8.0,
            0.0, 0.0, 0,
            Gesture::Step::ResetCondition{
                Gesture::Step::ResetCondition::Type::TimeoutAfterMs, 5000
            },
            std::nullopt,
            true
        },
        // Step 2: Threshold, move higher, value=12.0, no reset, do not take per-step picture
        {
            Gesture::Step::StepType::Threshold,
            Gesture::Step::MoveType::Higher,
            12.0,
            0.0, 0.0, 0,
            std::nullopt,
            std::nullopt,
            false
        }
    };

    // With randomize step picture (only call "picture" ONCE between the steps that request it)
    Gesture gesture1("TestGesture", "ThresholdOnly", 30.0, false, sequence, signal_index, std::nullopt, true);
    gesture1.set_picture_callback(print_picture_callback);
    gesture1.start();

    std::cout << "--- Threshold-only with RANDOMIZED per-step picture (should only print 'PICTURE' once for per-step):\n";
    std::cout << "Step1 (update with 11): " << (gesture1.update(11, signal_index) ? "Gesture detected!" : "No detection") << "\n";
    std::cout << "Step2 (update with 7): " << (gesture1.update(7, signal_index) ? "Gesture detected!" : "No detection") << "\n";
    std::cout << "Step3 (update with 13): " << (gesture1.update(13, signal_index) ? "Gesture detected!" : "No detection") << "\n";

    // TEST 2: per-step picture but NO randomization ("picture" called at every step that has .take_picture_at_the_end)
    Gesture gesture2("TestGestureNoRand", "ThresholdOnlyNoRand", 30.0, false, sequence, signal_index, std::nullopt, false);
    gesture2.set_picture_callback(print_picture_callback);
    gesture2.start();
    std::cout << "\n--- Threshold-only with PER-STEP picture (should print 'PICTURE' twice for steps, not for global):\n";
    std::cout << "Step1 (update with 11): " << (gesture2.update(11, signal_index) ? "Gesture detected!" : "No detection") << "\n";
    std::cout << "Step2 (update with 7): " << (gesture2.update(7, signal_index) ? "Gesture detected!" : "No detection") << "\n";
    std::cout << "Step3 (update with 13): " << (gesture2.update(13, signal_index) ? "Gesture detected!" : "No detection") << "\n";

    // TEST 3: Take a picture only on gesture completion, not per-step
    Gesture gesture3("TestGestureOnEnd", "GesturePictureOnly", 30.0, true, sequence, signal_index, std::nullopt, false);
    gesture3.set_picture_callback(print_picture_callback);
    gesture3.start();
    std::cout << "\n--- Picture on gesture only (should only print PICTURE at end):\n";
    gesture3.update(11, signal_index);
    gesture3.update(7, signal_index);
    gesture3.update(13, signal_index);

    // TEST 4: Range hold step with per-step picture
    std::vector<Gesture::Step> range_sequence = {
        // Range: Step, must be in [2.0, 4.0] for 2s, then take picture
        {
            Gesture::Step::StepType::Range,
            Gesture::Step::MoveType::Higher,  // move_to_next_type unused for range, set any
            0.0,                              // value unused for range, set any
            2.0, 4.0, 2000,                   // min_value, max_value, min_duration_ms
            Gesture::Step::ResetCondition{
                Gesture::Step::ResetCondition::Type::TimeoutAfterMs, 5000
            },
            std::nullopt,
            true          // take picture at this step completion
        },
        // Then threshold down
        {
            Gesture::Step::StepType::Threshold,
            Gesture::Step::MoveType::Lower,
            2.0,
            0.0, 0.0, 0,
            std::nullopt,
            std::nullopt,
            false
        }
    };
    Gesture hold_gesture("HoldInRange", "HoldRangePicture", 10.0, false, range_sequence, signal_index, std::nullopt, false);
    hold_gesture.set_picture_callback(print_picture_callback);
    hold_gesture.start();

    std::cout << "\n--- Range step with per-step picture. Should print picture after 2+ seconds in range:\n";
    std::cout << "Entering range (3.0): " << (hold_gesture.update(3.0, signal_index) ? "Gesture detected!" : "No detection") << "\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(2100));
    std::cout << "Still in range (3.2): " << (hold_gesture.update(3.2, signal_index) ? "Gesture detected!" : "No detection") << "\n";
    std::cout << "Below threshold (1.0): " << (hold_gesture.update(1.0, signal_index) ? "Gesture detected!" : "No detection") << "\n";
    std::cout << "Second step (1.5): " << (hold_gesture.update(1.5, signal_index) ? "Gesture detected!" : "No detection") << "\n";

    // Edge: No step has take_picture_at_the_end and gesture set to false: there should be no PICTURE
    std::vector<Gesture::Step> no_pic_seq = {
        {
            Gesture::Step::StepType::Threshold,
            Gesture::Step::MoveType::Higher,
            3.0,
            0.0, 0.0, 0,
            std::nullopt,
            std::nullopt,
            false
        },
        {
            Gesture::Step::StepType::Threshold,
            Gesture::Step::MoveType::Higher,
            4.0,
            0.0, 0.0, 0,
            std::nullopt,
            std::nullopt,
            false
        }
    };
    Gesture no_pic("NoPic", "NoPicture", 30.0, false, no_pic_seq, signal_index, std::nullopt, false);
    no_pic.set_picture_callback(print_picture_callback);
    no_pic.start();
    std::cout << "\n--- Steps and gesture with no take_picture_at_the_end (should see NO PICTURE outputs):\n";
    no_pic.update(4.0, signal_index);
    no_pic.update(5.0, signal_index);

    std::cout << "\nTest complete.\n";
    return 0;
}