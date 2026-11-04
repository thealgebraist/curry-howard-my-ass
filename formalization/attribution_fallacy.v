From Stdlib Require Import Logic.Classical.

(* Formalization of the Fundamental Attribution Error in System Diagnostics and Clinical Triage *)

Section DiagnosticAttribution.

  (* --- Part 1: The SSH Diagnostic Problem --- *)

  Variables (LocalClientRunning : Prop)  (* Local network interface, ssh-agent, local DNS *)
            (RemoteServerValid  : Prop)  (* Remote sshd, authorized_keys, remote port *)
            (ConnectionSuccess  : Prop). (* Observed successful connection *)

  (* Axiom: A connection succeeds if and only if BOTH local and remote conditions are satisfied *)
  Variable (H_connection : ConnectionSuccess <-> (LocalClientRunning /\ RemoteServerValid)).

  (* The Observation: The terminal displays an ERROR *)
  Variable (H_error : ~ ConnectionSuccess).

  (* The Ground Truth: The local setup is actually broken (e.g., local Wi-Fi / local agent dead) *)
  Variable (H_local_broken : ~ LocalClientRunning).

  (* 
     The LLM / Human Fallacy:
     Observing an ERROR and assuming the problem MUST be on the remote server,
     ignoring the local failure.
  *)
  Definition SSH_RemoteAttributionFallacy :=
    (~ ConnectionSuccess /\ ~ LocalClientRunning) -> ~ RemoteServerValid.

  (* 
     Formal Proof: The Remote Attribution Fallacy is NOT a valid rule of inference.
     Assuming it holds generally allows one to deduce False (a logical contradiction).
  *)
  Theorem ssh_attribution_is_fallacy :
    (forall L R S : Prop, (S <-> (L /\ R)) -> (~ S /\ ~ L -> ~ R)) -> False.
  Proof.
    intros FallacyRule.
    (* Instantiate with Local = False, Remote = True, Success = False *)
    assert (H_inst : False <-> (False /\ True)).
    { split; [ intros [] | intros [H _]; exact H ]. }
    specialize (FallacyRule False True False H_inst).
    assert (H_premise : ~ False /\ ~ False).
    { split; auto. }
    apply FallacyRule in H_premise.
    (* H_premise now asserts ~ True, which is False *)
    apply H_premise.
    exact I.
  Qed.

End DiagnosticAttribution.


Section ClinicalTransplantAttribution.

  (* --- Part 2: The Isomorphic Hospital Transplant Scenario --- *)

  Variables (HospitalPipelineValid : Prop) (* Local hospital refrigeration, sterile OR, correct reagent *)
            (PatientCompatible     : Prop) (* Patient biological/immunological compatibility *)
            (TransplantSuccess     : Prop). (* Successful graft acceptance *)

  (* Axiom: Successful transplant requires BOTH valid hospital delivery AND patient compatibility *)
  Variable (H_transplant : TransplantSuccess <-> (HospitalPipelineValid /\ PatientCompatible)).

  (* The Observation: Graft fails / transplant error *)
  Variable (H_clinical_failure : ~ TransplantSuccess).

  (* The Ground Truth: Hospital local pipeline failed (e.g., cold storage temp rose, reagent spoiled) *)
  Variable (H_hospital_pipeline_broken : ~ HospitalPipelineValid).

  (* 
     The Racial / Demographic Attribution Fallacy:
     When a transplant fails due to a local hospital pipeline defect,
     an observer wrongly attributes the failure to the patient's race/demographic,
     falsely concluding the patient's biology was incompatible.
  *)
  Definition DemographicAttributionFallacy :=
    (~ TransplantSuccess /\ ~ HospitalPipelineValid) -> ~ PatientCompatible.

  (* 
     Formal Proof: Blaming the patient's demographic for a local institutional failure
     is mathematically isomorphic to the SSH remote attribution error and equally unsound.
  *)
  Theorem demographic_attribution_is_fallacy :
    (forall Hospital Patient Outcome : Prop, 
      (Outcome <-> (Hospital /\ Patient)) -> 
      (~ Outcome /\ ~ Hospital -> ~ Patient)) -> False.
  Proof.
    intros FallacyRule.
    assert (H_inst : False <-> (False /\ True)).
    { split; [ intros [] | intros [H _]; exact H ]. }
    specialize (FallacyRule False True False H_inst).
    assert (H_premise : ~ False /\ ~ False).
    { split; auto. }
    apply FallacyRule in H_premise.
    apply H_premise.
    exact I.
  Qed.

  (* Sound Abductive Diagnostic Rule: What a Sherlock Holmes / Prolog engine actually infers *)
  Theorem sound_abduction :
    ~ (HospitalPipelineValid /\ PatientCompatible) -> ~ HospitalPipelineValid \/ ~ PatientCompatible.
  Proof.
    intros H_fail.
    destruct (classic HospitalPipelineValid) as [H_hosp | H_not_hosp].
    - right. intros H_pat. apply H_fail. split; assumption.
    - left. exact H_not_hosp.
  Qed.

  (* In constructive logic: if hospital pipeline fails, failure is guaranteed regardless of patient *)
  Theorem constructive_hospital_fault :
    ~ HospitalPipelineValid -> ~ (HospitalPipelineValid /\ PatientCompatible).
  Proof.
    intros H_hosp [H_hosp_ok _].
    apply H_hosp; exact H_hosp_ok.
  Qed.

End ClinicalTransplantAttribution.
