"""Deterministic runtime baseline and anomaly model for the Guardian simulator."""

# Import math for square-root standard deviations.
import math

# Import the shared protocol models.
from guardian_protocol import (
    DspFeatures,
    HealthState,
    HealthStatus,
)


# Define the same bounded feature floors used by firmware.
_SIGMA_FLOOR = (
    5.0,
    50.0,
    500.0,
    500.0,
    10.0,
    10.0,
    10.0,
)

# Define the same feature weights used by firmware.
_WEIGHT = (
    1.50,
    1.20,
    1.00,
    0.90,
    0.80,
    0.80,
    0.80,
)

# Define the firmware warning threshold.
_WARNING_Z = 3.0

# Define the firmware alarm threshold.
_ALARM_Z = 6.0

# Define the firmware recovery threshold.
_RECOVERY_Z = 2.0


# Implement a small simulator mirror of the firmware M8 model.
class SimulatorHealthModel:
    """Mirror M8 baseline and health behavior for software-only demos."""

    # Create one untrained health model.
    def __init__(self) -> None:

        # Initialize every field deterministically.
        self.reset()

    # Erase baseline and runtime anomaly state.
    def reset(self) -> None:
        """Return the simulator model to UNTRAINED."""

        # Start without a learned baseline.
        self.state = HealthState.UNTRAINED

        # Start without accepted baseline samples.
        self.baseline_samples = 0

        # Start without a baseline target.
        self.baseline_target = 0

        # Start with no sample-rate contract.
        self.sample_rate_hz = 0

        # Start every feature mean at zero.
        self.means = [0.0] * 7

        # Start every Welford squared-deviation accumulator at zero.
        self.m2 = [0.0] * 7

        # Start with neutral health.
        self.anomaly_score = 0

        # Start with maximum health score.
        self.health_score = 1000

        # Start without a dominant anomaly feature.
        self.dominant_feature = 0xFF

        # Start without anomaly persistence.
        self.anomalous_streak = 0

        # Start without recovery persistence.
        self.recovery_streak = 0

        # Start without model-quality warnings.
        self.quality_flags = 0

        # Start without a scored DSP block.
        self.block_sequence = 0

        # Start without current feature summaries.
        self.current_rms_mg = 0

        # Start without current crest factor.
        self.current_crest_factor_milli = 0

        # Start without current dominant frequency.
        self.current_dominant_frequency_centi_hz = 0

        # Start without exceeded feature bits.
        self.exceeded_feature_mask = 0

        # Start without rejected inputs.
        self.rejected_inputs = 0

        # Start without a published maximum deviation.
        self.max_deviation_milli = 0

    # Start a fresh bounded baseline.
    def start(self, target_samples: int) -> None:
        """Start a new explicit baseline session."""

        # Enforce the same firmware bounds.
        if not 16 <= target_samples <= 1024:

            # Reject a configuration firmware would reject.
            raise ValueError(
                "baseline target must be between 16 and 1024"
            )

        # Erase any previous model.
        self.reset()

        # Enter baseline learning.
        self.state = HealthState.LEARNING

        # Preserve the explicit target.
        self.baseline_target = target_samples

    # Convert one DSP snapshot into the seven firmware model features.
    @staticmethod
    def _vector(features: DspFeatures) -> tuple[float, ...]:
        """Return the fixed seven-feature model vector."""

        # Preserve feature order used by firmware masks.
        return (
            float(features.rms_mg),
            float(features.crest_factor_milli),
            float(features.dominant_frequency_centi_hz),
            float(features.spectral_centroid_centi_hz),
            float(features.low_band_permille),
            float(features.mid_band_permille),
            float(features.high_band_permille),
        )

    # Return the effective standard deviation for one feature.
    def _sigma(self, index: int) -> float:
        """Return measured sigma bounded by the firmware floor."""

        # Start with the configured floor.
        sigma = _SIGMA_FLOOR[index]

        # Calculate sample variance only after two baseline samples.
        if self.baseline_samples > 1:

            # Calculate Welford sample variance.
            variance = self.m2[index] / (
                self.baseline_samples - 1
            )

            # Use measured variability only when larger than the floor.
            if variance > 0.0:

                # Convert variance to standard deviation.
                sigma = max(
                    sigma,
                    math.sqrt(variance),
                )

        # Return the effective denominator.
        return sigma

    # Learn one trustworthy DSP feature vector.
    def ingest(self, features: DspFeatures) -> None:
        """Learn or score one deterministic DSP snapshot."""

        # Capture sample rate from the first baseline sample.
        if (
            self.state == HealthState.LEARNING
            and self.baseline_samples == 0
        ):

            # Preserve the first baseline sample rate.
            self.sample_rate_hz = features.sample_rate_hz

        # Reject sample-rate mismatch after a contract exists.
        if (
            self.sample_rate_hz
            and features.sample_rate_hz != self.sample_rate_hz
        ):

            # Saturate rejected-input diagnostics.
            self.rejected_inputs = min(
                0xFFFF,
                self.rejected_inputs + 1,
            )

            # Mark input rejection plus sample-rate mismatch.
            self.quality_flags |= 0x0002 | 0x0004

            # Return without contaminating the model.
            return

        # Learn only while baseline collection is active.
        if self.state == HealthState.LEARNING:

            # Convert the DSP snapshot into the model vector.
            vector = self._vector(features)

            # Calculate the sample count after this accepted update.
            next_count = self.baseline_samples + 1

            # Update every feature using Welford's method.
            for index, value in enumerate(vector):

                # Calculate deviation from the old mean.
                delta = value - self.means[index]

                # Update the running mean.
                self.means[index] += delta / next_count

                # Calculate deviation from the new mean.
                delta_after = value - self.means[index]

                # Update squared-deviation accumulator.
                self.m2[index] += delta * delta_after

            # Publish the accepted baseline count.
            self.baseline_samples = next_count

            # Publish the latest accepted learning block for operator visibility.
            self.block_sequence = features.block_sequence

            # Publish current RMS while baseline learning is active.
            self.current_rms_mg = features.rms_mg

            # Publish current crest factor while baseline learning is active.
            self.current_crest_factor_milli = (
                features.crest_factor_milli
            )

            # Publish current dominant frequency while baseline learning is active.
            self.current_dominant_frequency_centi_hz = (
                features.dominant_frequency_centi_hz
            )

            # Preserve the reference-calibration caveat used by simulator data.
            if features.acquisition_status_flags & 0x0010:

                # Mark the baseline as reference-calibrated.
                self.quality_flags |= 0x0001

            # Finalize automatically at the explicit target.
            if self.baseline_samples >= self.baseline_target:

                # Enter trained normal monitoring.
                self.state = HealthState.READY

                # Mark baseline readiness.
                self.quality_flags |= 0x0008

            # Learning samples are not anomaly-scored.
            return

        # Ignore scoring before a baseline exists.
        if self.state == HealthState.UNTRAINED:

            # Return without changing neutral status.
            return

        # Convert current DSP features.
        vector = self._vector(features)

        # Start without an exceeded feature.
        exceeded_mask = 0

        # Start without a dominant feature.
        dominant_feature = 0xFF

        # Start with zero normalized deviation.
        maximum_weighted_z = 0.0

        # Compare every feature against the frozen baseline.
        for index, value in enumerate(vector):

            # Calculate absolute deviation.
            deviation = abs(value - self.means[index])

            # Normalize by measured variability or configured floor.
            weighted = (
                deviation
                / self._sigma(index)
                * _WEIGHT[index]
            )

            # Publish warning-threshold feature bits.
            if weighted >= _WARNING_Z:

                # Set one bit per exceeded feature.
                exceeded_mask |= 1 << index

            # Preserve the worst feature.
            if weighted > maximum_weighted_z:

                # Update the maximum deviation.
                maximum_weighted_z = weighted

                # Preserve the feature index.
                dominant_feature = index

        # Preserve current block identity.
        self.block_sequence = features.block_sequence

        # Preserve current RMS.
        self.current_rms_mg = features.rms_mg

        # Preserve current crest factor.
        self.current_crest_factor_milli = (
            features.crest_factor_milli
        )

        # Preserve current dominant frequency.
        self.current_dominant_frequency_centi_hz = (
            features.dominant_frequency_centi_hz
        )

        # Publish exceeded feature mask.
        self.exceeded_feature_mask = exceeded_mask

        # Publish dominant anomaly feature.
        self.dominant_feature = dominant_feature

        # Publish bounded maximum deviation.
        self.max_deviation_milli = min(
            0xFFFF,
            round(maximum_weighted_z * 1000.0),
        )

        # Map six weighted sigma units to full anomaly severity.
        self.anomaly_score = min(
            1000,
            round(
                maximum_weighted_z
                / _ALARM_Z
                * 1000.0
            ),
        )

        # Publish inverse health score.
        self.health_score = 1000 - self.anomaly_score

        # Mark or clear current anomaly presence.
        if maximum_weighted_z >= _WARNING_Z:

            # Preserve current anomaly indication.
            self.quality_flags |= 0x0010
        else:

            # Clear only current anomaly indication.
            self.quality_flags &= ~0x0010

        # Qualify alarm-level persistence first.
        if maximum_weighted_z >= _ALARM_Z:

            # Reset recovery qualification.
            self.recovery_streak = 0

            # Advance anomaly persistence.
            self.anomalous_streak = min(
                255,
                self.anomalous_streak + 1,
            )

            # Enter ALARM after two severe blocks.
            if self.anomalous_streak >= 2:

                # Publish severe state.
                self.state = HealthState.ALARM
            elif self.state == HealthState.READY:

                # Escalate immediately to WARNING while ALARM qualifies.
                self.state = HealthState.WARNING
        elif maximum_weighted_z >= _WARNING_Z:

            # Reset recovery qualification.
            self.recovery_streak = 0

            # Advance warning persistence.
            self.anomalous_streak = min(
                255,
                self.anomalous_streak + 1,
            )

            # Enter WARNING after three persistent warning blocks.
            if (
                self.anomalous_streak >= 3
                and self.state != HealthState.ALARM
            ):

                # Publish warning state.
                self.state = HealthState.WARNING
        else:

            # Clear anomaly qualification below warning level.
            self.anomalous_streak = 0

            # Qualify recovery below the lower hysteresis threshold.
            if maximum_weighted_z < _RECOVERY_Z:

                # Advance recovery persistence.
                self.recovery_streak = min(
                    255,
                    self.recovery_streak + 1,
                )

                # Recover WARNING or ALARM after five normal blocks.
                if (
                    self.recovery_streak >= 5
                    and self.state
                    in (
                        HealthState.WARNING,
                        HealthState.ALARM,
                    )
                ):

                    # Return to normal trained monitoring.
                    self.state = HealthState.READY
            else:

                # Reset recovery inside the hysteresis band.
                self.recovery_streak = 0

    # Return the current immutable host-visible snapshot.
    def status(self) -> HealthStatus:
        """Return the current machine-health status."""

        # Publish learned RMS mean only after baseline samples exist.
        baseline_rms_mean_mg = (
            round(self.means[0])
            if self.baseline_samples
            else 0
        )

        # Publish the effective RMS sigma only after baseline samples exist.
        baseline_rms_std_mg = (
            round(self._sigma(0))
            if self.baseline_samples
            else 0
        )

        # Return one shared protocol model.
        return HealthStatus(
            state=self.state,
            baseline_samples=self.baseline_samples,
            baseline_target=self.baseline_target,
            anomaly_score=self.anomaly_score,
            health_score=self.health_score,
            max_deviation_milli=self.max_deviation_milli,
            dominant_feature=self.dominant_feature,
            consecutive_anomalous=self.anomalous_streak,
            quality_flags=self.quality_flags,
            block_sequence=self.block_sequence,
            current_rms_mg=self.current_rms_mg,
            current_crest_factor_milli=self.current_crest_factor_milli,
            current_dominant_frequency_centi_hz=(
                self.current_dominant_frequency_centi_hz
            ),
            baseline_rms_mean_mg=baseline_rms_mean_mg,
            baseline_rms_std_mg=baseline_rms_std_mg,
            exceeded_feature_mask=self.exceeded_feature_mask,
            rejected_inputs=self.rejected_inputs,
        )
